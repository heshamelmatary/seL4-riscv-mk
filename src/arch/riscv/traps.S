#include <config.h>
#include <machine/assembler.h>


# define LREG lw
# define SREG sw

.section .syscall, "ax"
  .align 6
  j trap_entry
  .align 6
  j trap_entry_s
  .align 6
  j trap_entry
  .align 6
  j trap_entry

.section .text

trap_entry:
  addi sp, sp, -272
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
  
  csrr a0, mcause
  csrr a1, mepc
  mv a2, sp
  jal handle_trap
  csrw mepc, a0
  
  LREG x1, 8(sp)
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

  eret

trap_entry_s:
  addi sp, sp, -272
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
  
  csrr a0, mcause
  csrr a1, mepc
  mv a2, sp
  jal handle_trap
  csrw mepc, a0
  
  LREG x1, 8(sp)
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

  eret

