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
#include <arch/machine.h>

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

volatile uint64_t magic_mem[8] __attribute__((aligned(64)));

static long syscall(long num, long arg0, long arg1, long arg2)
{
  register long a7 asm("a7") = num;
  register long a0 asm("a0") = arg0;
  register long a1 asm("a1") = arg1;
  register long a2 asm("a2") = arg2;
  asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));
  return a0;
}

void halt ()
{
  syscall(SYS_exit, 0, (long) 0, 0);
}

PHYS_CODE void tohost_exit(long code)
{
  write_csr(mtohost, (code << 1) | 1);
  while (1);
}

PHYS_CODE static long handle_frontend_syscall(long which, long arg0, long arg1, long arg2)
{
  magic_mem[0] = which;
  magic_mem[1] = arg0;
  magic_mem[2] = arg1;
  magic_mem[3] = arg2;
  __sync_synchronize();
  write_csr(mtohost, (long)magic_mem);
  while (swap_csr(mfromhost, 0) == 0);
  return magic_mem[0];
}

PHYS_CODE VISIBLE long handle_trap(uint32_t cause, uint32_t epc, uint64_t regs[32])
{
  int* csr_insn;
  //asm ("jal %0, 1f; csrr a0, stats; 1:" : "=r"(csr_insn));
  long sys_ret = 0;

  if(cause == CAUSE_FAULT_STORE)
  {
    printf("EXCEPTION: store fault epc = %x! \n", epc);
    
    halt();
  } 
  
  if (regs[17] == SYS_exit)
    tohost_exit(regs[10]);

  //else if (regs[17] == SYS_stats)
    //sys_ret = handle_stats(regs[10]);
  else
    sys_ret = handle_frontend_syscall(regs[17], regs[10], regs[11], regs[12]);

  regs[10] = sys_ret;
  return epc+4;
}

static uint32_t strlen(char *s)
{
  uint32_t counter = 0;
  while (*s++ != '\0')
    ++counter;
    
  return counter;
}

void printstr(char *s)
{  
  syscall(SYS_write, 1, (long) s, strlen(s));
}

int putchar(int ch)
{
  static __thread char buf[64] __attribute__((aligned(64)));
  static __thread int buflen = 0;
  buf[buflen++] = ch;
  if (ch == '\n' || buflen == sizeof(buf))
  {
    syscall(SYS_write, 1, (long)buf, buflen);
    buflen = 0;
  }
  return 0;
}

void
qemu_uart_putchar(char c)
{
  //putchar((int) c); 
}

void putDebugChar(unsigned char c)
{
  putchar(c);
}

unsigned char getDebugChar(void)
{
}

#endif
