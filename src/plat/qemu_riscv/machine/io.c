/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <stdint.h>
#include <util.h>
#include <machine/io.h>
#include <plat/machine/devices.h>

#define UART_REG(x)      (*((volatile char *) (x)))

#define UART_BASE        0x000003f8

#define UART_REG_TX              (UART_BASE+0)
#define UART_REG_RX              (UART_BASE+0)
#define UART_REG_DEV_LATCH_LOW   (UART_BASE+1)
#define UART_REG_DEV_LATCH_HIGH  (UART_BASE+1)
#define UART_REG_INT_ENABLE      (UART_BASE+2)
#define UART_REG_INT_ID          (UART_BASE+2)
#define UART_REG_FIFO_CTRL       (UART_BASE+2)
#define UART_REG_LINE_CTRL       (UART_BASE+3)
#define UART_REG_MODEM_CTRL      (UART_BASE+4)
#define UART_REG_LINE_STATUS     (UART_BASE+5)
#define UART_REG_MODEM_STATUS    (UART_BASE+6)
#define UART_REG_SCRATCH         (UART_BASE+7)

/* FIFO Control Register */
#define UART_REG_FIFO_CTRL_TRIGGER_1    (0x00)
#define UART_REG_FIFO_CTRL_ENABLE_FIFO  (0x01)
#define UART_REG_FIFO_CTRL_CLEAR_RCVR   (0x02)
#define UART_REG_FIFO_CTRL_CLEAR_XMIT   (0x03)
#define UART_REG_FIFO_CTRL_DMA_SELECT   (0x08)
#define UART_REG_FIFO_CTRL_TRIGGER_4    (0x40)
#define UART_REG_FIFO_CTRL_TRIGGER_8    (0x80)
#define UART_REG_FIFO_CTRL_TRIGGER_14   (0xC0)
#define UART_REG_FIFO_CTRL_TRIGGER_MASK (0xC0)

/* Line Control Register */
#define UART_REG_LINE_CTRL_WLEN5  (0x00)
#define UART_REG_LINE_CTRL_WLEN6  (0x01)
#define UART_REG_LINE_CTRL_WLEN7  (0x02)
#define UART_REG_LINE_CTRL_WLEN8  (0x03)
#define UART_REG_LINE_CTRL_STOP   (0x04)
#define UART_REG_LINE_CTRL_PARITY (0x08)
#define UART_REG_LINE_CTRL_EPAR   (0x10)
#define UART_REG_LINE_CTRL_SPAR   (0x20)
#define UART_REG_LINE_CTRL_SBC    (0x40)
#define UART_REG_LINE_CTRL_DLAB   (0x80)

/* Line Status Register */
#define UART_REG_LINE_STATUS_DR   (0x01)
#define UART_REG_LINE_STATUS_OE   (0x02)
#define UART_REG_LINE_STATUS_PE   (0x04)
#define UART_REG_LINE_STATUS_FE   (0x08)
#define UART_REG_LINE_STATUS_BI   (0x10)
#define UART_REG_LINE_STATUS_THRE (0x20)
#define UART_REG_LINE_STATUS_TEMT (0x40)

/* Modem Control Register */
#define UART_REG_MODEM_CTRL_DTR  (0x01)
#define UART_REG_MODEM_CTRL_RTS  (0x02)
#define UART_REG_MODEM_CTRL_OUT1 (0x04)
#define UART_REG_MODEM_CTRL_OUT2 (0x08)
#define UART_REG_MODEM_CTRL_LOOP (0x10)

/* Modem Status Register */
#define UART_REG_MODEM_STATUS_DCTS (0x01)
#define UART_REG_MODEM_STATUS_DDSR (0x02)
#define UART_REG_MODEM_STATUS_TERI (0x04)
#define UART_REG_MODEM_STATUS_DDCD (0x08)
#define UART_REG_MODEM_STATUS_CTS  (0x10)
#define UART_REG_MODEM_STATUS_DSR  (0x20)
#define UART_REG_MODEM_STATUS_RI   (0x40)
#define UART_REG_MODEM_STATUS_DCD  (0x80)
#define UART_REG_MODEM_STATUS_ANY_DELTA (0x0F)

static void uart_write_polled(char c)
{
  unsigned char lsr;
  const uint32_t transmit_finished =
    (UART_REG_LINE_STATUS_TEMT |
     UART_REG_LINE_STATUS_THRE);

  /* Wait until there is no pending data in the transmitter FIFO (empty) */
  do {
      lsr = UART_REG(UART_REG_LINE_STATUS);
  } while (!(lsr & UART_REG_LINE_STATUS_THRE));

  UART_REG(UART_REG_TX) = c;

  /* Wait until trasmit data is finished */
  do {
      lsr = UART_REG(UART_REG_LINE_STATUS);
  } while ( (lsr & transmit_finished) != transmit_finished );
}

#ifdef DEBUG

void
qemu_uart_putchar(char c)
{
  uart_write_polled(c);
}

void putDebugChar(unsigned char c)
{
  qemu_uart_putchar(c);
}

unsigned char getDebugChar(void)
{
}

#endif
