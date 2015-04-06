.section .boot.text, "ax"

.global __clzsi2;
.global __ctzsi2
.global _start

_start:
	nop
_loop:
  nop
  jal _loop
__clzsi2:
  nop

__ctzsi2:
  nop
