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

  addi sp, sp, -8

  sw   tp, 0(sp)
  csrr tp, sscratch 

  SREG x1, 4(tp)
  SREG x2, 8(tp)
  SREG x3, 12(tp)
  SREG x4, 16(tp)
  SREG x5, 20(tp)
  SREG x6, 24(tp)
  SREG x7, 28(tp)
  SREG x8, 32(tp)
  SREG x9, 36(tp)
  SREG x10, 40(tp)
  SREG x11, 44(tp)
  SREG x12, 48(tp)
  SREG x13, 52(tp)
  SREG x14, 56(tp)
  SREG x15, 60(tp)
  SREG x16, 64(tp)
  SREG x17, 68(tp)
  SREG x18, 72(tp)
  SREG x19, 76(tp)
  SREG x20, 80(tp)
  SREG x21, 84(tp)
  SREG x22, 88(tp)
  SREG x23, 92(tp)
  SREG x24, 96(tp)
  SREG x25, 100(tp)
  SREG x26, 104(tp)
  SREG x27, 108(tp)
  SREG x28, 112(tp)
  SREG x29, 116(tp)
  SREG x30, 120(tp)
  SREG x31, 124(tp)

  la sp, PPTR_KERNEL_STACK_TOP

  csrr s2,  sepc
  addi s2, s2, 4

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

ret_user:
  csrw sepc, s2
  
  LREG x1, 4(tp)
  LREG x2, 8(tp)
  LREG x3, 12(tp)
  LREG x4, 16(tp)
  LREG x5, 20(tp)
  LREG x6, 24(tp)
  LREG x7, 28(tp)
  LREG x8, 32(tp)
  LREG x9, 36(tp)
  LREG x10, 40(tp)
  LREG x11, 44(tp)
  LREG x12, 48(tp)
  LREG x13, 52(tp)
  LREG x14, 56(tp)
  LREG x15, 60(tp)
  LREG x16, 64(tp)
  LREG x17, 68(tp)
  LREG x18, 72(tp)
  LREG x19, 76(tp)
  LREG x20, 80(tp)
  LREG x21, 84(tp)
  LREG x22, 88(tp)
  LREG x23, 92(tp)
  LREG x24, 96(tp)
  LREG x25, 100(tp)
  LREG x26, 104(tp)
  LREG x27, 108(tp)
  LREG x28, 112(tp)
  LREG x29, 116(tp)
  LREG x30, 120(tp)
  LREG x31, 124(tp)

  lw   tp, 0(sp)
  addi sp, sp, 8

  eret
