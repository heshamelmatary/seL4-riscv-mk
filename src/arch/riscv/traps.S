#include <config.h>
#include <machine/assembler.h>

#include <arch/machine/hardware.h>

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

trap_entry:
  la tp, ksCurThread
  lw tp, 0(tp)

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

  /*addi sp, sp, -272
  SREG x1, 8(sp)
  SREG x2, 16(sp)
  SREG x3, 24(sp)
  SREG x4, 32(sp)
  SREG x5, 40(sp)
  SREG x6, 48(sp)
  SREG x7, 56(sp)
  SREG x8, 64(sp)
  SREG x9, 72(sp)
  SREG x10, 80(sp)
  SREG x11, 88(sp)
  SREG x12, 96(sp)
  SREG x13, 104(sp)
  SREG x14, 112(sp)
  SREG x15, 120(sp)
  SREG x16, 128(sp)
  SREG x17, 136(sp)
  SREG x18, 144(sp)
  SREG x19, 152(sp)
  SREG x20, 160(sp)
  SREG x21, 168(sp)
  SREG x22, 176(sp)
  SREG x23, 184(sp)
  SREG x24, 192(sp)
  SREG x25, 200(sp)
  SREG x26, 208(sp)
  SREG x27, 216(sp)
  SREG x28, 224(sp)
  SREG x29, 232(sp)
  SREG x30, 240(sp)
  SREG x31, 248(sp)
  */

  la sp, PPTR_KERNEL_STACK_TOP
  #csrr a0, mcause
  #csrr a1, mepc
  mv a0, a7
  mv a1, sp

  /* FIXME */
  la s0, handleUnknownSyscall
  jalr s0

  #csrw mepc, a0
  
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

  /*LREG x1, 8(sp)
  LREG x2, 16(sp)
  LREG x3, 24(sp)
  LREG x4, 32(sp)
  LREG x5, 40(sp)
  LREG x6, 48(sp)
  LREG x7, 56(sp)
  LREG x8, 64(sp)
  LREG x9, 72(sp)
  LREG x10, 80(sp)
  LREG x11, 88(sp)
  LREG x12, 96(sp)
  LREG x13, 104(sp)
  LREG x14, 112(sp)
  LREG x15, 120(sp)
  LREG x16, 128(sp)
  LREG x17, 136(sp)
  LREG x18, 144(sp)
  LREG x19, 152(sp)
  LREG x20, 160(sp)
  LREG x21, 168(sp)
  LREG x22, 176(sp)
  LREG x23, 184(sp)
  LREG x24, 192(sp)
  LREG x25, 200(sp)
  LREG x26, 208(sp)
  LREG x27, 216(sp)
  LREG x28, 224(sp)
  LREG x29, 232(sp)
  LREG x30, 240(sp)
  LREG x31, 248(sp)
  addi sp, sp, 272
  */
  eret
