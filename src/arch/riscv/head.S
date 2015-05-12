#define MSTATUS_IE          0x00000001
#define MSTATUS_PRV         0x00000006
#define MSTATUS_IE1         0x00000008
#define MSTATUS_PRV1        0x00000030
#define MSTATUS_IE2         0x00000040
#define MSTATUS_PRV2        0x00000180
#define MSTATUS_IE3         0x00000200
#define MSTATUS_PRV3        0x00000C00
#define MSTATUS_FS          0x00003000
#define MSTATUS_XS          0x0000C000
#define MSTATUS_MPRV        0x00010000
#define MSTATUS_VM          0x003E0000
#define MSTATUS32_SD        0x80000000
#define MSTATUS64_SD        0x8000000000000000

.section .vector, "ax"
  j _start

.section .boot.text, "ax"
.global __clzsi2;
.global __ctzsi2
.global _start
.extern init_kernel
.extern riscv_kernel_stack


_start:  
  li x1, 0
  li x2, 0
  li x3, 0
  li x4, 0
  li x5, 0
  li x6, 0
  li x7, 0
  li x8, 0
  li x9, 0
  li x10,0
  li x11,0
  li x12,0
  li x13,0
  li x14,0
  li x15,0
  li x16,0
  li x17,0
  li x18,0
  li x19,0
  li x20,0
  li x21,0
  li x22,0
  li x23,0
  li x24,0
  li x25,0
  li x26,0
  li x27,0
  li x28,0
  li x29,0
  li x30,0
  li x31,0
 
  #li t0, 0x00000002; csrc mstatus, t0 # run tests in supervisor mode
  li t0, MSTATUS_IE; csrs mstatus, t0 # enable interrupts
  
  la a0, init_kernel

  /* Set the first PTE - Test */
  /*la s1, pg_dir
  ori s2, s2, 0b111000101
  sw s2, 0(s1)
  */
  /* Enable VM */
  /*li s0, 0x80
	csrs status, s0
  */
  /* Try to write to unmapped address */
  /*add s2, x0, x0
  li s2, 0x0600000
  
  
  li x31, 0x1
  sw x31, 0(s2)
  */
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
