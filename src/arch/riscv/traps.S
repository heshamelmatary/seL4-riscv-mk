#include <config.h>
#include <machine/assembler.h>

# define LREG lw
# define SREG sw

.section .boot.text, "ax"
.global _start
.global trap_entry
.extern handleUnknownSyscall
.extern global_start

/* This would be mapped at 0xFFC00000 and called as is from elfloader (M-mode) when there's a syscall from the user*/

_start:
  la s0, global_start
  jr s0

.section .vector, "ax"
.balign 4096

#include <arch/api/syscall.h>
#include <arch/machine/hardware.h>

#include <arch/machine/registerset.h>

trap_entry:

  csrr t0, sscratch 

  SREG x1, 4(t0)
  SREG x2, 8(t0)
  SREG x3, 12(t0)
  SREG x4, 16(t0)
  SREG x5, 20(t0)
  SREG x6, 24(t0)
  SREG x7, 28(t0)
  SREG x8, 32(t0)
  SREG x9, 36(t0)
  SREG x10, 40(t0)
  SREG x11, 44(t0)
  SREG x12, 48(t0)
  SREG x13, 52(t0)
  SREG x14, 56(t0)
  SREG x15, 60(t0)
  SREG x16, 64(t0)
  SREG x17, 68(t0)
  SREG x18, 72(t0)
  SREG x19, 76(t0)
  SREG x20, 80(t0)
  SREG x21, 84(t0)
  SREG x22, 88(t0)
  SREG x23, 92(t0)
  SREG x24, 96(t0)
  SREG x25, 100(t0)
  SREG x26, 104(t0)
  SREG x27, 108(t0)
  SREG x28, 112(t0)
  SREG x29, 116(t0)
  SREG x30, 120(t0)
  SREG x31, 124(t0)

  la sp, PPTR_KERNEL_STACK_TOP

  csrr s2,  sepc
  addi s2, s2, 4

  /* Check if it's an interrupt */
  csrr s0, scause
  srli s0, s0, 31
  li   s1, 0x1
  beq  s0, s1, handle_irq

  /* Move systemcall number to a0 */
  mv a0, a7

  /* Check that syscall number is in range */
  li  s0, SYSCALL_MIN
  blt a0, s0, unknownsyscall
  nop
  la  s0, handleSyscall
  jalr s0
  nop
  j   ret_user

unknownsyscall:
  /* FIXME */
  la s0, handleUnknownSyscall
  jalr s0
  j   ret_user

handle_irq:
  la   s0, handleInterruptEntry
  jalr s0

ret_user:
  csrw sepc, s2

  la t0, ksCurThread
  lw t0, 0(t0)

  LREG x1, 4(t0)
  LREG x2, 8(t0)
  LREG x3, 12(t0)
  LREG x4, 16(t0)
  #LREG x5, 20(t0)
  LREG x6, 24(t0)
  LREG x7, 28(t0)
  LREG x8, 32(t0)
  LREG x9, 36(t0)
  LREG x10, 40(t0)
  LREG x11, 44(t0)
  LREG x12, 48(t0)
  LREG x13, 52(t0)
  LREG x14, 56(t0)
  LREG x15, 60(t0)
  LREG x16, 64(t0)
  LREG x17, 68(t0)
  LREG x18, 72(t0)
  LREG x19, 76(t0)
  LREG x20, 80(t0)
  LREG x21, 84(t0)
  LREG x22, 88(t0)
  LREG x23, 92(t0)
  LREG x24, 96(t0)
  LREG x25, 100(t0)
  LREG x26, 104(t0)
  LREG x27, 108(t0)
  LREG x28, 112(t0)
  LREG x29, 116(t0)
  LREG x30, 120(t0)
  LREG x31, 124(t0)

  csrr t1, sscratch
  beq  t0, t1, _normal_exit #ksCurThread has changes, jump to the new one 
_context_switch:
  csrw sepc, ra

_normal_exit:
  csrw sscratch, t0
  LREG x5, 20(t0) # t0

  eret
