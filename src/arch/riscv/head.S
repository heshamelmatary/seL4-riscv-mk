#define MSTATUS_PRV         0x00000060
#define MSTATUS_SSIP        0x00000002
#define MSTATUS_HSIP        0x00000004
#define MSTATUS_MSIP        0x00000008
#define MSTATUS_IE          0x00000010
#define MSTATUS_PRV         0x00000060
#define MSTATUS_IE1         0x00000080
#define MSTATUS_PRV1        0x00000300
#define MSTATUS_IE2         0x00000400
#define MSTATUS_PRV2        0x00001800
#define MSTATUS_IE3         0x00002000
#define MSTATUS_PRV3        0x0000C000
#define MSTATUS_MPRV        0x00030000
#define MSTATUS_VM          0x00780000
#define MSTATUS_STIE        0x01000000
#define MSTATUS_HTIE        0x02000000
#define MSTATUS_MTIE        0x04000000
#define MSTATUS_FS          0x18000000
#define MSTATUS_XS          0x60000000
#define MSTATUS32_SD        0x80000000
#define MSTATUS64_UA        0x0000000F00000000
#define MSTATUS64_SA        0x000000F000000000
#define MSTATUS64_HA        0x00000F0000000000
#define MSTATUS64_SD        0x8000000000000000

#define SSTATUS_SIP         0x00000002
#define SSTATUS_IE          0x00000010
#define SSTATUS_PIE         0x00000080
#define SSTATUS_PS          0x00000100
#define SSTATUS_UA          0x000F0000
#define SSTATUS_TIE         0x01000000
#define SSTATUS_TIP         0x04000000
#define SSTATUS_FS          0x18000000
#define SSTATUS_XS          0x60000000
#define SSTATUS32_SD        0x80000000
#define SSTATUS64_SD        0x8000000000000000

#define PRV_U 0
#define PRV_S 1
#define PRV_H 2
#define PRV_M 3

#define VM_MBARE 0
#define VM_MBB   1
#define VM_MBBID 2
#define VM_SV32  4
#define VM_SV39  5
#define VM_SV48  6

#define UA_RV32  0
#define UA_RV64  4
#define UA_RV128 8

#define IRQ_TIMER  0
#define IRQ_IPI    1
#define IRQ_HOST   2
#define IRQ_COP    3

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
  /* 32-bit mode */
  /*li s0, 0x1f0
	csrc status, s0

  la s0, pg_dir
	csrw ptbr, s0
	csrw fatc, 0
  */
  /*csrs mstatus, MSTATUS_PRV1*/
  
  #li t0, MSTATUS_PRV; csrs mstatus, t0 # run tests in user mode
  li t0, MSTATUS_IE1; csrs mstatus, t0 # enable interrupts in user mode
  #li t0, MSTATUS_FS; csrc mstatus, t0 # enable FPU
  #li t0, MSTATUS_XS; csrc mstatus, t0 # enable accelerator
  li t0, ((MSTATUS64_UA & ~(MSTATUS64_UA << 1)) * UA_RV64) >> 31
  #sll t0, t0, 31
  li t1, ((MSTATUS64_SA & ~(MSTATUS64_SA << 1)) * UA_RV64) >> 31
  sll t1, t1, 31
  
  #ifdef __riscv64
  # make sure processor supports RV64 if this was compiled for RV64
  #bnez t0, 1f
  #li a0, 1234
  #j tohost_exit
  #1:
  # enable RV64 for user and supervisor
  #csrs mstatus, t0
  #csrs mstatus, t1
  #else
  # disable RV64 for user and supervisor
  csrc mstatus, t0
  csrc mstatus, t1
  #endif
  /*csrr t0, mstatus
  li t1, MSTATUS_XS
  and t1, t0, t1
  sw t1, have_vec, t2
  */
  
  la sp, (riscv_kernel_stack + 4096)
  la s0, (init_kernel)
  /*csrw sup0, s0*/
  /*csrw mscratch, sp*/
  li a1, 1337
  
  
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
