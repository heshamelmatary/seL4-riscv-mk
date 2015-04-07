.section .boot.text, "ax"
.global __clzsi2;
.global __ctzsi2
.global _start
.extern init_kernel
.extern riscv_kernel_stack

_start:

  la sp, (riscv_kernel_stack + 4096)
  la s0, init_kernel
  csrw sup0, s0
  la s0, init_kernel
  jr s0

  nop
  
_loop:
  nop

jal _loop
  __clzsi2:
  nop
  __ctzsi2:
  nop

