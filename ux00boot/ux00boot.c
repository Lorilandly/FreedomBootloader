/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdatomic.h>
#include <string.h>
#include <encoding.h>
#include <sifive/platform.h>
#include <sifive/bits.h>
#include <sifive/smp.h>
#include <spi/spi.h>
#include <uart/uart.h>
#include <gpt/gpt.h>
#include <sd/sd.h>
#include "ux00boot.h"


#define GPT_BLOCK_SIZE 512

// Bit fields of error codes
#define ERROR_CODE_BOOTSTAGE (0xfUL << 60)
#define ERROR_CODE_TRAP (0xfUL << 56)
#define ERROR_CODE_ERRORCODE ((0x1UL << 56) - 1)
// Bit fields of mcause fields when compressed to fit into the errorcode field
#define ERROR_CODE_ERRORCODE_MCAUSE_INT (0x1UL << 55)
#define ERROR_CODE_ERRORCODE_MCAUSE_CAUSE ((0x1UL << 55) - 1)

#define ERROR_CODE_UNHANDLED_SPI_DEVICE 0x1
#define ERROR_CODE_UNHANDLED_BOOT_ROUTINE 0x2
#define ERROR_CODE_GPT_PARTITION_NOT_FOUND 0x3
#define ERROR_CODE_SPI_COPY_FAILED 0x4
#define ERROR_CODE_SD_CARD_CMD0 0x5
#define ERROR_CODE_SD_CARD_CMD8 0x6
#define ERROR_CODE_SD_CARD_ACMD41 0x7
#define ERROR_CODE_SD_CARD_CMD58 0x8
#define ERROR_CODE_SD_CARD_CMD16 0x9
#define ERROR_CODE_SD_CARD_CMD18 0xa
#define ERROR_CODE_SD_CARD_CMD18_CRC 0xb
#define ERROR_CODE_SD_CARD_UNEXPECTED_ERROR 0xc

// error LED not implemented
// We are assuming that an error LED is connected to the GPIO pin
#define UX00BOOT_ERROR_LED_GPIO_PIN 15
#define UX00BOOT_ERROR_LED_GPIO_MASK (1 << 15)


//==============================================================================
// UX00 boot routine functions
//==============================================================================

//------------------------------------------------------------------------------
// SD Card
//------------------------------------------------------------------------------

static int initialize_sd(spi_ctrl* spictrl)
{
  int error = sd_init(spictrl);
  if (error) {
    switch (error) {
      case SD_INIT_ERROR_CMD0: return ERROR_CODE_SD_CARD_CMD0;
      case SD_INIT_ERROR_CMD8: return ERROR_CODE_SD_CARD_CMD8;
      case SD_INIT_ERROR_ACMD41: return ERROR_CODE_SD_CARD_ACMD41;
      case SD_INIT_ERROR_CMD58: return ERROR_CODE_SD_CARD_CMD58;
      case SD_INIT_ERROR_CMD16: return ERROR_CODE_SD_CARD_CMD16;
      default: return ERROR_CODE_SD_CARD_UNEXPECTED_ERROR;
    }
  }
  uart_puts((void*)UART0_CTRL_ADDR, "SD initialization complete!\n\r");
  return 0;
}

static gpt_partition_range find_sd_gpt_partition(
  spi_ctrl* spictrl,
  uint64_t partition_entries_lba,
  uint32_t num_partition_entries,
  uint32_t partition_entry_size,
  const gpt_guid* partition_type_guid,
  void* block_buf  // Used to temporarily load blocks of SD card
)
{
  // Exclusive end
  uint64_t partition_entries_lba_end = (
    partition_entries_lba +
    (num_partition_entries * partition_entry_size + GPT_BLOCK_SIZE - 1) / GPT_BLOCK_SIZE
  );
  for (uint64_t i = partition_entries_lba; i < partition_entries_lba_end; i++) {
    sd_copy(spictrl, block_buf, i, 1);
    gpt_partition_range range = gpt_find_partition_by_guid(
      block_buf, partition_type_guid, GPT_BLOCK_SIZE / partition_entry_size
    );
    if (gpt_is_valid_partition_range(range)) {
      return range;
    }
  }
  return gpt_invalid_partition_range();
}


static int decode_sd_copy_error(int error)
{
  switch (error) {
    case SD_COPY_ERROR_CMD18: return ERROR_CODE_SD_CARD_CMD18;
    case SD_COPY_ERROR_CMD18_CRC: return ERROR_CODE_SD_CARD_CMD18_CRC;
    default: return ERROR_CODE_SD_CARD_UNEXPECTED_ERROR;
  }
}


static int load_sd_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
{
  uint8_t gpt_buf[GPT_BLOCK_SIZE];
  int error;
  error = sd_copy(spictrl, gpt_buf, GPT_HEADER_LBA, 1);
  if (error) return decode_sd_copy_error(error);

  gpt_partition_range part_range;
  {
    // header will be overwritten by find_sd_gpt_partition(), so locally
    // scope it.
    gpt_header* header = (gpt_header*) gpt_buf;
    part_range = find_sd_gpt_partition(
      spictrl,
      header->partition_entries_lba,
      header->num_partition_entries,
      header->partition_entry_size,
      partition_type_guid,
      gpt_buf
    );
  }

  if (!gpt_is_valid_partition_range(part_range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }

  error = sd_copy(
    spictrl,
    dst,
    part_range.first_lba,
    part_range.last_lba + 1 - part_range.first_lba
  );
  if (error) return decode_sd_copy_error(error);
  uart_puts((void*)UART0_CTRL_ADDR, "SD Load Partition Complete!\n\r");
  return 0;
}


//------------------------------------------------------------------------------
// SPI flash
//------------------------------------------------------------------------------

/**
 * Set up SPI for direct, non-memory-mapped access.
 *
static inline int initialize_spi_flash_direct(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  // Max desired SPI clock is 10MHz
  spictrl->sckdiv = spi_min_clk_divisor(spi_clk_input_khz, 10000);

  spictrl->fctrl.en = 0;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_RESET_ENABLE);
  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_MEMORY_RESET);

  return 0;
}


static inline int _initialize_spi_flash_mmap(spi_ctrl* spictrl, unsigned int spi_clk_input_khz, unsigned int pad_cnt, unsigned int data_proto, unsigned int command_code)
{
  // Max desired SPI clock is 10MHz
  spictrl->sckdiv = spi_min_clk_divisor(spi_clk_input_khz, 10000);

  spictrl->fctrl.en = 0;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_RESET_ENABLE);
  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_MEMORY_RESET);

  spictrl->ffmt.raw_bits = ((spi_reg_ffmt) {
    .cmd_en = 1,
    .addr_len = 3,
    .pad_cnt = pad_cnt,
    .command_proto = SPI_PROTO_S,
    .addr_proto = SPI_PROTO_S,
    .data_proto = data_proto,
    .command_code = command_code,
  }).raw_bits;

  spictrl->fctrl.en = 1;
  __asm__ __volatile__ ("fence io, io");
  return 0;
}


static int initialize_spi_flash_mmap_single(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  return _initialize_spi_flash_mmap(spictrl, spi_clk_input_khz, 0, SPI_PROTO_S, MICRON_SPI_FLASH_CMD_READ);
}


static int initialize_spi_flash_mmap_quad(spi_ctrl* spictrl, unsigned int spi_clk_input_khz)
{
  return _initialize_spi_flash_mmap(spictrl, spi_clk_input_khz, 8, SPI_PROTO_Q, MICRON_SPI_FLASH_CMD_QUAD_FAST_READ);
}


//------------------------------------------------------------------------------
// SPI flash memory-mapped

static gpt_partition_range find_mmap_gpt_partition(const void* gpt_base, const gpt_guid* partition_type_guid)
{
  gpt_header* header = (gpt_header*) ((uintptr_t) gpt_base + GPT_HEADER_LBA * GPT_BLOCK_SIZE);
  gpt_partition_range range;
  range = gpt_find_partition_by_guid(
    (const void*) ((uintptr_t) gpt_base + header->partition_entries_lba * GPT_BLOCK_SIZE),
    partition_type_guid,
    header->num_partition_entries
  );
  if (gpt_is_valid_partition_range(range)) {
    return range;
  }
  return gpt_invalid_partition_range();
}


/**
 * Load GPT partition from memory-mapped GPT image.
 *
static int load_mmap_gpt_partition(const void* gpt_base, void* payload_dest, const gpt_guid* partition_type_guid)
{
  gpt_partition_range range = find_mmap_gpt_partition(gpt_base, partition_type_guid);
  if (!gpt_is_valid_partition_range(range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }
  memcpy(
    payload_dest,
    (void*) ((uintptr_t) gpt_base + range.first_lba * GPT_BLOCK_SIZE),
    (range.last_lba + 1 - range.first_lba) * GPT_BLOCK_SIZE
  );
  return 0;
}


//------------------------------------------------------------------------------
// SPI flash non-memory-mapped

static gpt_partition_range find_spiflash_gpt_partition(
  spi_ctrl* spictrl,
  uint64_t partition_entries_lba,
  uint32_t num_partition_entries,
  uint32_t partition_entry_size,
  const gpt_guid* partition_type_guid,
  void* block_buf  // Used to temporarily load blocks of SD card
)
{
  // Exclusive end
  uint64_t partition_entries_lba_end = (
    partition_entries_lba +
    (num_partition_entries * partition_entry_size + GPT_BLOCK_SIZE - 1) / GPT_BLOCK_SIZE
  );
  for (uint64_t i = partition_entries_lba; i < partition_entries_lba_end; i++) {
    spi_copy(spictrl, block_buf, i * GPT_BLOCK_SIZE, GPT_BLOCK_SIZE);
    gpt_partition_range range = gpt_find_partition_by_guid(
      block_buf, partition_type_guid, GPT_BLOCK_SIZE / partition_entry_size
    );
    if (gpt_is_valid_partition_range(range)) {
      return range;
    }
  }
  return gpt_invalid_partition_range();
}


/**
 * Load GPT partition from SPI flash.
 *
static int load_spiflash_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid)
{
  uint8_t gpt_buf[GPT_BLOCK_SIZE];
  int error;
  error = spi_copy(spictrl, gpt_buf, GPT_HEADER_LBA * GPT_BLOCK_SIZE, GPT_HEADER_BYTES);
  if (error) return ERROR_CODE_SPI_COPY_FAILED;

  gpt_partition_range part_range;
  {
    gpt_header* header = (gpt_header*) gpt_buf;
    part_range = find_spiflash_gpt_partition(
      spictrl,
      header->partition_entries_lba,
      header->num_partition_entries,
      header->partition_entry_size,
      partition_type_guid,
      gpt_buf
    );
  }

  if (!gpt_is_valid_partition_range(part_range)) {
    return ERROR_CODE_GPT_PARTITION_NOT_FOUND;
  }

  error = spi_copy(
    spictrl,
    dst,
    part_range.first_lba * GPT_BLOCK_SIZE,
    (part_range.last_lba + 1 - part_range.first_lba) * GPT_BLOCK_SIZE
  );
  if (error) return ERROR_CODE_SPI_COPY_FAILED;
  return 0;
}
*/


void ux00boot_fail(long code, int trap)
{
  if (read_csr(mhartid) == NONSMP_HART) {
    // Print error code to UART
    UART0_REG(UART_REG_CTRL) = UART_RST_TX;

    // Error codes are formatted as follows:
    // [63:60]    [59:56]  [55:0]
    // bootstage  trap     errorcode
    // If trap == 1, then errorcode is actually the mcause register with the
    // interrupt bit shifted to bit 55.
    uint64_t error_code = 0;
    if (trap) {
      error_code = INSERT_FIELD(error_code, ERROR_CODE_ERRORCODE_MCAUSE_CAUSE, code);
      if (code < 0) {
        error_code = INSERT_FIELD(error_code, ERROR_CODE_ERRORCODE_MCAUSE_INT, 0x1UL);
      }
    } else {
      error_code = code;
    }
    uint64_t formatted_code = 0;
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_BOOTSTAGE, UX00BOOT_BOOT_STAGE);
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_TRAP, trap);
    formatted_code = INSERT_FIELD(formatted_code, ERROR_CODE_ERRORCODE, error_code);

    uart_puts((void*) UART0_CTRL_ADDR, "Error 0x");
    uart_put_hex((void*) UART0_CTRL_ADDR, formatted_code >> 32);
    uart_put_hex((void*) UART0_CTRL_ADDR, formatted_code);
  }

  // Turn on LED
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_VAL), UX00BOOT_ERROR_LED_GPIO_MASK);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_EN), UX00BOOT_ERROR_LED_GPIO_MASK);
  atomic_fetch_or(&GPIO_REG(GPIO_OUTPUT_XOR), UX00BOOT_ERROR_LED_GPIO_MASK);

  while (1);
}


//==============================================================================
// Public functions
//==============================================================================

/**
 * Load GPT partition match specified partition type into specified memory.
 *
 * Read from mode select device to determine which bulk storage medium to read
 * GPT image from, and properly initialize the bulk storage based on type.
 */
void ux00boot_load_gpt_partition(void* dst, const gpt_guid* partition_type_guid)
{

  spi_ctrl* spictrl = (spi_ctrl*) SPI_CTRL_ADDR;
  unsigned int error = 0;
  error = initialize_sd(spictrl);
  if (!error) error = load_sd_gpt_partition(spictrl, dst, partition_type_guid);

  if (error) {
    ux00boot_fail(error, 0);
  }
}
