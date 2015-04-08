.section .boot.text, "ax"
.global __clzsi2;
.global __ctzsi2
.global _start
.global 
.extern init_kernel
.extern riscv_kernel_stack

_start:

  /* 32-bit mode */
  li s0, 0x1f0
	csrc status, s0

  la s0, pg_dir
	csrw ptbr, s0
	csrw fatc, 0

  la sp, (riscv_kernel_stack + 4096)
  la s0, (init_kernel)
  csrw sup0, s0

  /* Set the first PTE - Test */
  la s1, pg_dir
  ori s2, s2, 0b111000101
  sw s2, 0(s1)

  /* Enable VM */
  li s0, 0x80
	csrs status, s0

  /* Try to write to unmapped address */
  add s2, x0, x0
  li s2, 0x0600000

  li x31, 0x1
  sw x31, 0(s2)

/*_loop:
  nop
  jal _loop
  nop
 */ 

  la s0, init_kernel
  jr s0

__clzsi2:
  nop
__ctzsi2:
  nop

.balign 0x1000
pg_dir:
.fill (pg_dir + 0x1000) - ., 1, 0x00
.end    pg_dir
.size   pg_dir, .-pg_dir
