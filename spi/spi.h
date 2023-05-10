/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _DRIVERS_SPI_H
#define _DRIVERS_SPI_H

#ifndef __ASSEMBLER__

#include <stdint.h>

#define _ASSERT_SIZEOF(type, size) _Static_assert(sizeof(type) == (size), #type " must be " #size " bytes wide")

typedef union
{
  struct
  {
    uint32_t pha : 1;
    uint32_t pol : 1;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_sckmode;
_ASSERT_SIZEOF(spi_reg_sckmode, 4);


typedef union
{
  struct
  {
    uint32_t mode : 2;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_csmode;
_ASSERT_SIZEOF(spi_reg_csmode, 4);


typedef union
{
  struct
  {
    uint32_t cssck : 8;
    uint32_t reserved0 : 8;
    uint32_t sckcs : 8;
    uint32_t reserved1 : 8;
  };
  uint32_t raw_bits;
} spi_reg_delay0;
_ASSERT_SIZEOF(spi_reg_delay0, 4);


typedef union
{
  struct
  {
    uint32_t intercs : 8;
    uint32_t reserved0 : 8;
    uint32_t interxfr : 8;
    uint32_t reserved1 : 8;
  };
  uint32_t raw_bits;
} spi_reg_delay1;
_ASSERT_SIZEOF(spi_reg_delay1, 4);


typedef union
{
  struct
  {
    uint32_t proto : 2;
    uint32_t endian : 1;
    uint32_t dir : 1;
    uint32_t reserved0 : 12;
    uint32_t len : 4;
    uint32_t reserved1 : 12;
  };
  uint32_t raw_bits;
} spi_reg_fmt;
_ASSERT_SIZEOF(spi_reg_fmt, 4);


typedef union
{
  struct
  {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t full : 1;
  };
  uint32_t raw_bits;
} spi_reg_txdata;
_ASSERT_SIZEOF(spi_reg_txdata, 4);


typedef union
{
  struct
  {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t empty : 1;
  };
  uint32_t raw_bits;
} spi_reg_rxdata;
_ASSERT_SIZEOF(spi_reg_rxdata, 4);


typedef union
{
  struct
  {
    uint32_t txmark : 3;
    uint32_t reserved : 29;
  };
  uint32_t raw_bits;
} spi_reg_txmark;
_ASSERT_SIZEOF(spi_reg_txmark, 4);


typedef union
{
  struct
  {
    uint32_t rxmark : 3;
    uint32_t reserved : 29;
  };
  uint32_t raw_bits;
} spi_reg_rxmark;
_ASSERT_SIZEOF(spi_reg_rxmark, 4);


typedef union
{
  struct
  {
    uint32_t en : 1;
    uint32_t reserved : 31;
  };
  uint32_t raw_bits;
} spi_reg_fctrl;
_ASSERT_SIZEOF(spi_reg_fctrl, 4);


typedef union
{
  struct
  {
    uint32_t cmd_en : 1;
    uint32_t addr_len : 3;
    uint32_t pad_cnt : 4;
    uint32_t command_proto : 2;
    uint32_t addr_proto : 2;
    uint32_t data_proto : 2;
    uint32_t reserved : 2;
    uint32_t command_code : 8;
    uint32_t pad_code : 8;
  };
  uint32_t raw_bits;
} spi_reg_ffmt;
_ASSERT_SIZEOF(spi_reg_ffmt, 4);


typedef union
{
  struct
  {
    uint32_t txwm : 1;
    uint32_t rxwm : 1;
    uint32_t reserved : 30;
  };
  uint32_t raw_bits;
} spi_reg_ie;
typedef spi_reg_ie spi_reg_ip;
_ASSERT_SIZEOF(spi_reg_ie, 4);
_ASSERT_SIZEOF(spi_reg_ip, 4);

typedef union
{
  struct
  {
    uint32_t reserved : 22;
    uint32_t lsb : 1;
    uint32_t transaction_inhibit : 1;
    uint32_t manual_slave_select : 1;
    uint32_t rx_rst : 1;
    uint32_t tx_rst : 1;
    uint32_t cpha : 1;
    uint32_t cpol : 1;
    uint32_t master : 1;
    uint32_t spe : 1;
    uint32_t loop : 1;
  };
  uint32_t raw_bits;
} spi_ctrl_reg;
_ASSERT_SIZEOF(spi_ctrl_reg, 4);

typedef union
{
  struct
  {
    uint32_t reserved : 21;
    uint32_t command_err : 1;
    uint32_t loopback_err : 1;
    uint32_t lsb_err : 1;
    uint32_t slave_mode_err : 1;
    uint32_t cpol_cpha_err : 1;
    uint32_t slave_mode_select : 1;
    uint32_t modf : 1;
    uint32_t tx_full : 1;
    uint32_t tx_empty : 1;
    uint32_t rx_full : 1;
    uint32_t rx_empty : 1;
  };
  uint32_t raw_bits;
} spi_stat_reg;
_ASSERT_SIZEOF(spi_stat_reg, 4);

#undef _ASSERT_SIZEOF


/**
 * SPI control register memory map.
 *
 * All functions take a pointer to a SPI device's control registers.
 */
typedef volatile struct
{
  uint32_t reserved00;
  uint32_t reserved04;
  uint32_t reserved08;
  uint32_t reserved0c;
  uint32_t reserved10;
  uint32_t reserved14;
  uint32_t reserved18;
  uint32_t dgier;       // 0x1c;
  uint32_t ipisr;       // 0x20;
  uint32_t reserved24;
  uint32_t ipier;       // 0x28;
  uint32_t reserved2c;
  uint32_t reserved30;
  uint32_t reserved34;
  uint32_t reserved38;
  uint32_t reserved3c;
  uint32_t srr;         // 0x40
  uint32_t reserved44;
  uint32_t reserved48;
  uint32_t reserved4c;
  uint32_t reserved50;
  uint32_t reserved54;
  uint32_t reserved58;
  uint32_t reserved5c;
  spi_ctrl_reg cr;      // 0x60
  spi_stat_reg sr;      // 0x64;
  uint32_t tx;          // 0x68;
  uint32_t rx;          // 0x6c;
  uint32_t ssr;         // 0x70;
  uint32_t tor;         // 0x74;
  uint32_t ror;         // 0x78;
  uint32_t reserved7c;
} spi_ctrl;
/**
typedef volatile struct
{
  uint32_t sckdiv;
  spi_reg_sckmode sckmode;
  uint32_t reserved08;
  uint32_t reserved0c;

  uint32_t csid;
  uint32_t csdef;
  spi_reg_csmode csmode;
  uint32_t reserved1c;

  uint32_t reserved20;
  uint32_t reserved24;
  spi_reg_delay0 delay0;
  spi_reg_delay1 delay1;

  uint32_t reserved30;
  uint32_t reserved34;
  uint32_t reserved38;
  uint32_t reserved3c;

  spi_reg_fmt fmt;
  uint32_t reserved44;
  spi_reg_txdata txdata;
  spi_reg_rxdata rxdata;

  spi_reg_txmark txmark;
  spi_reg_rxmark rxmark;
  uint32_t reserved58;
  uint32_t reserved5c;

  spi_reg_fctrl fctrl;
  spi_reg_ffmt ffmt;
  uint32_t reserved68;
  uint32_t reserved6c;

  spi_reg_ie ie;
  spi_reg_ip ip;
} spi_ctrl;
*/


void spi_tx(spi_ctrl* spictrl, uint8_t in);
uint8_t spi_rx(spi_ctrl* spictrl);
uint8_t spi_txrx(spi_ctrl* spictrl, uint8_t in);
int spi_copy(spi_ctrl* spictrl, void* buf, uint32_t addr, uint32_t size);


// Inlining header functions in C
// https://stackoverflow.com/a/23699777/7433423

/**
 * Get smallest clock divisor that divides input_khz to a quotient less than or
 * equal to max_target_khz;
 */
inline unsigned int spi_min_clk_divisor(unsigned int input_khz, unsigned int max_target_khz)
{
  // f_sck = f_in / (2 * (div + 1)) => div = (f_in / (2*f_sck)) - 1
  //
  // The nearest integer solution for div requires rounding up as to not exceed
  // max_target_khz.
  //
  // div = ceil(f_in / (2*f_sck)) - 1
  //     = floor((f_in - 1 + 2*f_sck) / (2*f_sck)) - 1
  //
  // This should not overflow as long as (f_in - 1 + 2*f_sck) does not exceed
  // 2^32 - 1, which is unlikely since we represent frequencies in kHz.
  unsigned int quotient = (input_khz + 2 * max_target_khz - 1) / (2 * max_target_khz);
  // Avoid underflow
  if (quotient == 0) {
    return 0;
  } else {
    return quotient - 1;
  }
}

#endif /* !__ASSEMBLER__ */

#endif /* _DRIVERS_SPI_H */
