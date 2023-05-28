/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include <sifive/barrier.h>
#include <sifive/platform.h>
#include <sifive/smp.h>
#include <ux00boot/ux00boot.h>
#include <gpt/gpt.h>
#include "encoding.h"
#include <uart/uart.h>


static Barrier barrier;
extern const gpt_guid gpt_guid_sifive_fsbl;


void handle_trap(void)
{
  ux00boot_fail((long) read_csr(mcause), 1);
}

/* no-op */
int puts(const char* str){
	return 1;
}

int main()
{
  if (read_csr(mhartid) == NONSMP_HART) {
    // change CCACHE_SIDEBAND_ADDR to 0x8000_0000
    uart_puts((void*)UART0_CTRL_ADDR, "\n\r");
    ux00boot_load_gpt_partition((void*) MEMORY_MEM_ADDR, &gpt_guid_sifive_fsbl);
    uart_puts((void*)UART0_CTRL_ADDR, "load gpt partition done!\n\r");
  }

  Barrier_Wait(&barrier, NUM_CORES);
  uart_putc((void*)UART0_CTRL_ADDR, '@');

  return 0;
}
