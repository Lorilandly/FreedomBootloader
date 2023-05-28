/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <stdint.h>
#include <sifive/platform.h>
#include "spi.h"


/**
 * Wait until SPI is ready for transmission and transmit byte.
 */
void spi_tx(spi_ctrl* spictrl, uint8_t in)
{
  while (spictrl->tor >> 24 >= 0x0f); // make sure tx does not overflow the FIFO buffer
  spictrl->tx = in;
}


/**
 * Wait until SPI receive queue has data and read byte.
 */
uint8_t spi_rx(spi_ctrl* spictrl)
{
  while (!spictrl->ror); // do not read when buffer is empty
  return spictrl->rx >> 24;
}


/**
 * Transmit a byte and receive a byte.
 */
uint8_t spi_txrx(spi_ctrl* spictrl, uint8_t in)
{
  spi_tx(spictrl, in);
  return spi_rx(spictrl);
}


#define MICRON_SPI_FLASH_CMD_RESET_ENABLE        0x66
#define MICRON_SPI_FLASH_CMD_MEMORY_RESET        0x99
#define MICRON_SPI_FLASH_CMD_READ                0x03
#define MICRON_SPI_FLASH_CMD_QUAD_FAST_READ      0x6b

/**
 * Copy data from SPI flash without memory-mapped flash.
 */
int spi_copy(spi_ctrl* spictrl, void* buf, uint32_t addr, uint32_t size)
{
  uint8_t* buf_bytes = (uint8_t*) buf;
  spictrl->cr.transaction_inhibit = 1;

  spi_txrx(spictrl, MICRON_SPI_FLASH_CMD_READ);
  spi_txrx(spictrl, (addr >> 16) & 0xff);
  spi_txrx(spictrl, (addr >> 8) & 0xff);
  spi_txrx(spictrl, addr & 0xff);

  for (unsigned int i = 0; i < size; i++) {
    *buf_bytes = spi_txrx(spictrl, 0);
    buf_bytes++;
  }

  spictrl->cr.transaction_inhibit = 0;
  return 0;
}


extern inline unsigned int spi_min_clk_divisor(unsigned int input_khz, unsigned int max_target_khz);
