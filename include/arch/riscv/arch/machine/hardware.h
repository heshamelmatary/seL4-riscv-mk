#ifndef __ARCH_MACHINE_HARDWARE_H
#define __ARCH_MACHINE_HARDWARE_H

#include <config.h>
#include <arch/types.h>
#include <arch/linker.h>

/* Include cache, MMU related functions */

#ifndef __ASSEMBLER__

#include <arch/types.h>

#define PAGE_BITS 12

// page table entry (PTE) fields
#define PTE_V     0x001 // Valid
#define PTE_TYPE  0x01E // Type
#define PTE_R     0x020 // Referenced
#define PTE_D     0x040 // Dirty
#define PTE_SOFT  0x380 // Reserved for Software

#define PTE_TYPE_TABLE        0x00
#define PTE_TYPE_TABLE_GLOBAL 0x02
#define PTE_TYPE_URX_SR       0x04
#define PTE_TYPE_URWX_SRW     0x06
#define PTE_TYPE_UR_SR        0x08
#define PTE_TYPE_URW_SRW      0x0A
#define PTE_TYPE_URX_SRX      0x0C
#define PTE_TYPE_URWX_SRWX    0x0E
#define PTE_TYPE_SR           0x10
#define PTE_TYPE_SRW          0x12
#define PTE_TYPE_SRX          0x14
#define PTE_TYPE_SRWX         0x16
#define PTE_TYPE_SR_GLOBAL    0x18
#define PTE_TYPE_SRW_GLOBAL   0x1A
#define PTE_TYPE_SRX_GLOBAL   0x1C
#define PTE_TYPE_SRWX_GLOBAL  0x1E

#define PTE_PPN_SHIFT 10
#define PTE_PPN1_SHIFT 20

/* Virtual address to index conforming sv32 PTE format */ 
#define VIRT1_TO_IDX(addr) ((addr) >> 22)
#define VIRT0_TO_IDX(addr) (((addr) >> 10) & 0x000003FF)

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

enum vm_fault_type {
    RISCVLoadAccessFault = 5,
    RISCVStoreAccessFault = 7 
};
typedef uint32_t vm_fault_type_t;

enum vm_page_size {
    RISCVNormalPage, /* 4KiB */
    RISCVMegaPage,   /* 4MiB */
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
