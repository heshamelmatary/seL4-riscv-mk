#include <config.h>
#include <machine/assembler.h>


.section .boot.text, "ax"
.global _start
.global trap_entry
.extern handleUnknownSyscall
.extern global_start

/* This would be mapped at 0xFFC00000 and called as is from elfloader (M-mode) when there's a syscall from the user*/

#define LREG ld
#define SREG sd
#define REGBYTES 8

_start:
1:
  auipc gp, %pcrel_hi(_gp)
  addi gp, gp, %pcrel_lo(1b)
  la s0, global_start
  jr s0

.section .vector, "ax"
.balign 4096

#include <arch/api/syscall.h>
#include <arch/machine/hardware.h>

#include <arch/machine/registerset.h>

trap_entry:

  csrr t0, sscratch 

  SREG x1, 1*REGBYTES(t0)
  SREG x2, 2*REGBYTES(t0)
  SREG x3, 3*REGBYTES(t0)
  SREG x4, 4*REGBYTES(t0)
  SREG x5, 5*REGBYTES(t0)
  SREG x6, 6*REGBYTES(t0)
  SREG x7, 7*REGBYTES(t0)
  SREG x8, 8*REGBYTES(t0)
  SREG x9, 9*REGBYTES(t0)
  SREG x10, 10*REGBYTES(t0)
  SREG x11, 11*REGBYTES(t0)
  SREG x12, 12*REGBYTES(t0)
  SREG x13, 13*REGBYTES(t0)
  SREG x14, 14*REGBYTES(t0)
  SREG x15, 15*REGBYTES(t0)
  SREG x16, 16*REGBYTES(t0)
  SREG x17, 17*REGBYTES(t0)
  SREG x18, 18*REGBYTES(t0)
  SREG x19, 19*REGBYTES(t0)
  SREG x20, 20*REGBYTES(t0)
  SREG x21, 21*REGBYTES(t0)
  SREG x22, 22*REGBYTES(t0)
  SREG x23, 23*REGBYTES(t0)
  SREG x24, 24*REGBYTES(t0)
  SREG x25, 25*REGBYTES(t0)
  SREG x26, 26*REGBYTES(t0)
  SREG x27, 27*REGBYTES(t0)
  SREG x28, 28*REGBYTES(t0)
  SREG x29, 29*REGBYTES(t0)
  SREG x30, 30*REGBYTES(t0)
  SREG x31, 31*REGBYTES(t0)

  csrr x1, sstatus
  SREG x1, 32*REGBYTES(t0)

  la sp, _kernel_stack 
  /* FIXME */
/*  lui s0, 0x4FFF1
  li  sp, 0xFAE
  add sp, s0, sp
*/
  /* Check if it's an interrupt */
  csrr s0, scause
  csrr s3, scause

  srli s0, s0, 31
  li   s1, 0x1
  beq  s0, s1, handle_irq

  andi s3, s3, 0xf
  li   s4, 8
  blt  s3, s4, _error
  j _syscall

_error:
  j _error
    /* Set the return address to sepc + 4 in the case of a systemcall */
_syscall:
  csrr s2,  sepc
  addi s2, s2, 4
  csrw sepc, s2

  /* Move systemcall number to a0 */
  mv a0, a7

  /* Check that syscall number is in range */
  li  s0, SYSCALL_MIN
  blt a0, s0, unknownsyscall
  nop
  la  s0, handleSyscall
  jalr s0
  SREG s2, 1*REGBYTES(t0)
  nop
  j   ret_user

unknownsyscall:
  la s0, handleUnknownSyscall
  jalr s0
  j   ret_user

handle_irq:
  la   s0, handleInterruptEntry
  jalr s0

ret_user:

  la t0, ksCurThread
  lw t0, 0(t0)

  LREG x1, 32*REGBYTES(t0)
  csrw sstatus, x1 

  LREG x1, 1*REGBYTES(t0)
  LREG x2, 2*REGBYTES(t0)
  LREG x3, 3*REGBYTES(t0)
  LREG x4, 4*REGBYTES(t0)
  /* skip x5/t0 */
  LREG x6, 6*REGBYTES(t0)
  LREG x7, 7*REGBYTES(t0)
  LREG x8, 8*REGBYTES(t0)
  LREG x9, 9*REGBYTES(t0)
  LREG x10, 10*REGBYTES(t0)
  LREG x11, 11*REGBYTES(t0)
  LREG x12, 12*REGBYTES(t0)
  LREG x13, 13*REGBYTES(t0)
  LREG x14, 14*REGBYTES(t0)
  LREG x15, 15*REGBYTES(t0)
  LREG x16, 16*REGBYTES(t0)
  LREG x17, 17*REGBYTES(t0)
  LREG x18, 18*REGBYTES(t0)
  LREG x19, 19*REGBYTES(t0)
  LREG x20, 20*REGBYTES(t0)
  LREG x21, 21*REGBYTES(t0)
  LREG x22, 22*REGBYTES(t0)
  LREG x23, 23*REGBYTES(t0)
  LREG x24, 24*REGBYTES(t0)
  LREG x25, 25*REGBYTES(t0)
  LREG x26, 26*REGBYTES(t0)
  LREG x27, 27*REGBYTES(t0)
  LREG x28, 28*REGBYTES(t0)
  LREG x29, 29*REGBYTES(t0)
  LREG x30, 30*REGBYTES(t0)
  LREG x31, 31*REGBYTES(t0)

  /* Check if there is a new higher priority task has taken control and 
   * if so, set sepc to ra and not sepc + 4
   */
  csrr t1, sscratch
  beq  t0, t1, _normal_exit #ksCurThread has changes, jump to the new one 
_context_switch:
  csrw sepc, ra

_normal_exit:
  csrw sscratch, t0
  LREG x5, 5*REGBYTES(t0) # t0

  eret
