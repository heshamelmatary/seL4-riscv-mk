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

.section .boot.text, "ax"
.global __clzsi2;
.global __ctzsi2
.global global_start
.extern init_kernel
.extern riscv_kernel_stack
.extern trap_entry

global_start:
  #li t0, 0x00000004; csrc mstatus, t0 # run in supervisor mode
  #li t0, MSTATUS_IE; csrs mstatus, t0 # enable interrupts
  la t0, (0x80000000)
  auipc s0, (0x80000)
  addi  x1, x0, -1
  la gp, 0
  la tp, test_area
  

  la sp, (riscv_kernel_stack)
  jal init_kernel
  
  /* Load initial thread context */
  la tp, ksCurThread
  lw tp, 0(tp)
  lw sp, 8(tp)
  /* Load a0 (bi_frame) */
  lw a0, 40(tp)
  
  lw sp, 0(tp)
  eret
