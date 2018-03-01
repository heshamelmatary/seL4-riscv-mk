/*
 * Copyright 2015 Hesham Almatary <heshamelmatary@gmail.com>
 * Author: Hesham Almatary <heshamelmatary@gmail.com>
 */

#ifndef __ARCH_MACHINE_HARDWARE_H
#define __ARCH_MACHINE_HARDWARE_H

/* Include cache, MMU related functions */
#define PAGE_BITS 12

#define PPTR_VECTOR_TABLE 0x40000000
#ifdef CONFIG_ROCKET_CHIP
#define PPTR_GLOBALS_PAGE 0x4FE01000
#else
#define PPTR_GLOBALS_PAGE 0x4FC01000
#endif
/* The stack is the very last page of virtual memory. */
#define PPTR_KERNEL_STACK 0x4FFF1000
#define PPTR_KERNEL_STACK_TOP (PPTR_KERNEL_STACK + 0x1000 - 16)

/* page table entry (PTE) fields */
#define PTE_V     0x001 // Valid
#define PTE_TYPE  0x01E // Type
#define PTE_R     0x020 // Referenced
#define PTE_D     0x040 // Dirty
#define PTE_SOFT  0x380 // Reserved for Software

#define PTE_PPN_SHIFT 10
#define PTE_PPN1_SHIFT 20

#define PTE64_PPN2_SHIFT 28
#define PTE64_PPN1_SHIFT 19
#define PTE64_PPN0_SHIFT 10 

#define SV39_VIRT_TO_VPN2(addr) ((addr) >> 30)
#define SV39_VIRT_TO_VPN1(addr) ((addr) >> 21)
#define SV39_VIRT_TO_VPN0(addr) ((addr) >> 12)

/* Virtual address to index conforming sv32 PTE format */ 
#define VIRT1_TO_IDX(addr) ((addr) >> 22)
#define VIRT0_TO_IDX(addr) (((addr) >> 12) & 0x000003FF)

#define PTE_TABLE(PTE) ((0x0000000AU >> ((PTE) & 0x1F)) & 1)
#define PTE_UR(PTE)    ((0x0000AAA0U >> ((PTE) & 0x1F)) & 1)
#define PTE_UW(PTE)    ((0x00008880U >> ((PTE) & 0x1F)) & 1)
#define PTE_UX(PTE)    ((0x0000A0A0U >> ((PTE) & 0x1F)) & 1)
#define PTE_SR(PTE)    ((0xAAAAAAA0U >> ((PTE) & 0x1F)) & 1)
#define PTE_SW(PTE)    ((0x88888880U >> ((PTE) & 0x1F)) & 1)
#define PTE_SX(PTE)    ((0xA0A0A000U >> ((PTE) & 0x1F)) & 1)

#define PTE_CHECK_PERM(PTE, SUPERVISOR, STORE, FETCH) \
  ((STORE) ? ((SUPERVISOR) ? PTE_SW(PTE) : PTE_UW(PTE)) : \
   (FETCH) ? ((SUPERVISOR) ? PTE_SX(PTE) : PTE_UX(PTE)) : \
             ((SUPERVISOR) ? PTE_SR(PTE) : PTE_UR(PTE)))

#define PROT_TO_PERM(PROT) ((((PROT) & PROT_EXEC) ? 2 : 0) | (((PROT) & PROT_WRITE) ? 1 : 0))

#define PTE_CREATE(PPN, TYPE) \
  (((PPN) << PTE_PPN_SHIFT) | (TYPE) | PTE_V)

#define TIMER_TICK_NS 0x1C9C380

#ifndef __ASSEMBLER__

#include <arch/types.h>
#include <config.h>
#include <arch/types.h>
#include <arch/linker.h>

enum vm_fault_type {
    RISCVLoadAccessFault = 5,
    RISCVStoreAccessFault = 7 
};
typedef uint32_t vm_fault_type_t;

enum vm_page_size {
    RISCVNormalPage, /* 4KiB */
    RISCVMegaPage,   /* 4MiB */
};

enum pte_type {
  RISCV_PTE_TYPE_TABLE = 0,
  RISCV_PTE_TYPE_TABLE_GLOBAL = 1,
  RISCV_PTE_TYPE_URX_SR = 2,
  RISCV_PTE_TYPE_URWX_SRW = 3,
  RISCV_PTE_TYPE_UR_SR = 4,
  RISCV_PTE_TYPE_URW_SRW = 5,
  RISCV_PTE_TYPE_URX_SRX = 6,
  RISCV_PTE_TYPE_URWX_SRWX = 7,
  RISCV_PTE_TYPE_SR = 8,
  RISCV_PTE_TYPE_SRW = 9,
  RISCV_PTE_TYPE_SRX = 10,
  RISCV_PTE_TYPE_SRWX = 11,
  RISCV_PTE_TYPE_SR_GLOBAL = 12,
  RISCV_PTE_TYPE_SRW_GLOBAL = 13,
  RISCV_PTE_TYPE_SRX_GLOBAL = 14,
  RISCV_PTE_TYPE_SRWX_GLOBAL = 15
};

typedef uint32_t vm_page_size_t;

enum PageSizeConstants {
    RISCVNormalPageBits   = 12, 
    RISCVMegaPageBits     = 22, 
};

static inline unsigned int CONST
pageBitsForSize(vm_page_size_t pagesize)
{
    switch (pagesize) {
    case RISCVNormalPage:
        return RISCVNormalPageBits;

    case RISCVMegaPage:
        return RISCVMegaPageBits;

    default:
        fail("Invalid page size");
    }
}

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_MACHINE_HARDWARE_H */
