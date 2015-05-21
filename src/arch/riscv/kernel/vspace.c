/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <types.h>
#include <benchmark.h>
#include <api/failures.h>
#include <api/syscall.h>
#include <kernel/boot.h>
#include <kernel/cspace.h>
#include <kernel/thread.h>
#include <machine/io.h>
#include <model/preemption.h>
#include <model/statedata.h>
#include <object/cnode.h>
#include <object/untyped.h>
#include <arch/api/invocation.h>
#include <arch/kernel/vspace.h>
#include <arch/linker.h>
#include <arch/object/tcb.h>
#include <plat/machine/devices.h>
#include <plat/machine/hardware.h>

pde_t l1pt[PTES_PER_PT] __attribute__((aligned(1024*1024*4))) PHYS_DATA VISIBLE;
pte_t l2pt[PTES_PER_PT] __attribute__((aligned(1024*1024*4))) PHYS_DATA VISIBLE;
/* This is only needed for 64-bit implementation, keep it for future */
uint32_t l3pt[PTES_PER_PT] __attribute__((aligned(4096))) PHYS_DATA VISIBLE;

char riscv_kernel_stack[1024*1024] __attribute__((aligned(4096))) BOOT_DATA;

struct resolve_ret {
    paddr_t frameBase;
    vm_page_size_t frameSize;
    bool_t valid;
};
typedef struct resolve_ret resolve_ret_t;

void doFlush(int label, vptr_t start, vptr_t end, paddr_t pstart);
static pte_t *lookupPTSlot_nofail(pte_t *pt, vptr_t vptr);
static resolve_ret_t resolveVAddr(pde_t *pd, vptr_t vaddr);
static exception_t performPDFlush(int label, pde_t *pd, asid_t asid,
                                  vptr_t start, vptr_t end, paddr_t pstart);
static exception_t performPageFlush(int label, pde_t *pd, asid_t asid,
                                    vptr_t start, vptr_t end, paddr_t pstart);
static exception_t performPageGetAddress(void *vbase_ptr);

static pde_t PURE loadHWASID(asid_t asid);

static bool_t PURE pteCheckIfMapped(pte_t *pte);
static bool_t PURE pdeCheckIfMapped(pde_t *pde);

/* The following two functions are used during bootstraping and before MMU
 * enable. They are copied from structures_gen.h but placed into physical 
 * memory.
 */
PHYS_CODE static inline pde_t __attribute__((__const__))
pde_new_phys(uint32_t ppn1, uint32_t sw, uint32_t dirty, uint32_t read, uint32_t type, uint32_t valid) {
    pde_t pde;

    pde.words[0] = 0;

    pde.words[0] |= (ppn1 & 0x00000fff) << 20;
    pde.words[0] |= (sw & 0x7) << 7;
    pde.words[0] |= (dirty & 0x1) << 6;
    pde.words[0] |= (read & 0x1) << 5;
    pde.words[0] |= (type & 0xf) << 1;
    pde.words[0] |= (valid & 0x1) << 0;

    return pde;
}

PHYS_CODE static inline pte_t __attribute__((__const__))
pte_new_phys(uint32_t ppn1, uint32_t ppn0, uint32_t sw, uint32_t dirty, uint32_t read, uint32_t type, uint32_t valid) {
    pte_t pte;

    pte.words[0] = 0;

    pte.words[0] |= (ppn1 & 0x00000fff) << 20;
    pte.words[0] |= (ppn0 & 0x3ff) << 10;
    pte.words[0] |= (sw & 0x7) << 7;
    pte.words[0] |= (dirty & 0x1) << 6;
    pte.words[0] |= (read & 0x1) << 5;
    pte.words[0] |= (type & 0xf) << 1;
    pte.words[0] |= (valid & 0x1) << 0;

    return pte;
}

PHYS_CODE VISIBLE void
map_kernel_window(void)
{
    paddr_t  phys;
    uint32_t idx, limit;
    pde_t    pde;
    long     i;

    /* mapping of kernelBase (virtual address) to kernel's physBase  */
    /* up to end of virtual address space minus 4MB */
    phys = VIRT1_TO_IDX(0x00400000);
    idx  = VIRT1_TO_IDX(kernelBase);
    limit = idx + 63;
 /* for(i = 0; i < idx ; i++)
  {
    l1pt[i] = pde_new_phys(i,
            0,
            0,
            0,
            RISCV_PTE_TYPE_SRWX,
            1);
  }
*/
//PTE_CREATE(i << PTE_PPN_SHIFT, PTE_TYPE_SRWX);
     
  /*  4 MB Mega Pages that covers 256 MiB - total memory */
  for(i = 0; idx < limit ; idx++, phys++)
  {
    l1pt[idx] = pde_new(
            phys,
            0,  /* sw */
            0,  /* dirty */ 
            0,  /* read */
            RISCV_PTE_TYPE_SRWX, /* type */
            1 /* valid */
     ); 
  
  
  //PTE_CREATE(phys << PTE_PPN_SHIFT, PTE_TYPE_SRWX);
   // PTE_CREATE(phys << PTE_PPN_SHIFT, 0x16); 
            
  }

  /* point to the next last 4MB physical page index */
  phys++;
  idx++;

  assert((phys << 22) == PADDR_TOP);

  //l1pt[VIRT1_TO_IDX(kernelBase) + 1] = PTE_CREATE(1 << PTE_PPN_SHIFT, PTE_TYPE_SRWX);
  //map_kernel_frame(&test_area, &test_area, PTE_TYPE_SRWX);

  write_csr(sptbr, addrFromPPtr(l1pt));

}

static word_t CONST
APFromVMRights(vm_rights_t vm_rights)
{
        return 0;

}

BOOT_CODE void
map_it_pt_cap(cap_t pd_cap)
{
}

BOOT_CODE void
map_kernel_frame(paddr_t paddr, pptr_t vaddr, vm_rights_t vm_rights)
{

    /* First level page table */
    uint32_t idx1 = VIRT1_TO_IDX(vaddr);
    uint32_t idx2 = VIRT0_TO_IDX(vaddr);

    //l1pt[idx1] = PTE_CREATE(((uint32_t) &l2pt) / RISCV_PGSIZE, PTE_TYPE_TABLE_GLOBAL);
    //l2pt[idx2] = PTE_CREATE(paddr, PTE_TYPE_SRWX_GLOBAL);

    /*assert(vaddr >= PPTR_TOP); /* vaddr lies in the region the global PT covers */
   
}

BOOT_CODE void
activate_global_pd(void)
{
}

BOOT_CODE void
write_it_asid_pool(cap_t it_ap_cap, cap_t it_pd_cap)
{
}

/* ==================== BOOT CODE FINISHES HERE ==================== */

void
copyGlobalMappings(pde_t *newPD)
{
}

word_t * PURE
lookupIPCBuffer(bool_t isReceiver, tcb_t *thread)
{
}

/*findPDForASID_ret_t
findPDForASID(asid_t asid)
{
}
*/
lookupPTSlot_ret_t
lookupPTSlot(pde_t *pd, vptr_t vptr)
{
}

static pte_t *
lookupPTSlot_nofail(pte_t *pt, vptr_t vptr)
{
}

pde_t * CONST
lookupPDSlot(pde_t *pd, vptr_t vptr)
{
}

exception_t
handleVMFault(tcb_t *thread, vm_fault_type_t vm_faultType)
{
}

/*static void
invalidateASID(asid_t asid)
{
}

static void
invalidateASIDEntry(asid_t asid)
{
}

void
deleteASIDPool(asid_t asid_base, asid_pool_t* pool)
{
}

void
deleteASID(asid_t asid, pde_t* pd)
{
}
*/
pde_t *
pageTableMapped(asid_t asid, vptr_t vaddr, pte_t* pt)
{
}

void unmapPageTable(pde_t* pd, uint32_t pdIndex)
{
    pd[pdIndex] = pde_new(
                      0,  /* ppn1 */
                      0,  /* sw */
                      0,  /* dirty */
                      0,  /* read */
                      0,  /* type */
                      0  /* valid */
                  );
}

static pte_t pte_pte_invalid_new(void)
{
}

void
unmapPage(vm_page_size_t page_size, asid_t asid, vptr_t vptr, void *pptr)
{
}

void
setVMRoot(tcb_t *tcb)
{
}

static bool_t
setVMRootForFlush(pde_t* pd, asid_t asid)
{
}

bool_t CONST
isValidVTableRoot(cap_t cap)
{
}

exception_t
checkValidIPCBuffer(vptr_t vptr, cap_t cap)
{
}

vm_rights_t CONST
maskVMRights(vm_rights_t vm_rights, cap_rights_t cap_rights_mask)
{
}

/*static void
storeHWASID(asid_t asid, hw_asid_t hw_asid)
{
}

static pde_t PURE
loadHWASID(asid_t asid)
{
}

hw_asid_t
findFreeHWASID(void)
{
}

static hw_asid_t
getHWASID(asid_t asid)
{
}

void
setCurrentASID(asid_t asid)
{
}
*/
/* Cache and TLB consistency */

void flushPage(vm_page_size_t page_size, pde_t* pd, word_t vptr)
{
}

 void flushTable(pde_t* pd, word_t vptr, pte_t* pt)
{
}

void
flushSpace(asid_t asid)
{
}

void
invalidateTLBByASID(asid_t asid)
{
}

/* The rest of the file implements the RISCV object invocations */

static pte_t CONST
makeUserPTE(vm_page_size_t page_size, paddr_t paddr,
            bool_t cacheable, bool_t nonexecutable, vm_rights_t vm_rights)
{
}

static pde_t CONST
makeUserPDE(vm_page_size_t page_size, paddr_t paddr, bool_t parity,
            bool_t cacheable, bool_t nonexecutable, word_t domain,
            vm_rights_t vm_rights)
{
}

static inline bool_t CONST
checkVPAlignment(vm_page_size_t sz, word_t w)
{
}

static exception_t
decodeRISCVPageTableInvocation(word_t label, unsigned int length,
                             cte_t *cte, cap_t cap, extra_caps_t extraCaps,
                             word_t *buffer)
{
    word_t          vaddr;
    vm_attributes_t attr;
    cap_t           vspaceCap;
    void*           vspace;
    pde_t           pde;
    pde_t *         pd, *pdSlot;
    paddr_t         paddr;

    if (label == RISCVPageTableUnmap) {
        setThreadState(ksCurThread, ThreadState_Restart);

        pd = PDE_PTR(cap_page_table_cap_get_capPTMappedObject(cap));
        if (pd) {
            pte_t *pt = PTE_PTR(cap_page_table_cap_get_capPTBasePtr(cap));
            uint32_t pdIndex = cap_page_table_cap_get_capPTMappedIndex(cap);
            unmapPageTable(pd, pdIndex);
            flushTable(pd, pdIndex, pt);
            clearMemory((void *)pt, cap_get_capSizeBits(cap));
        }
        cdtUpdate(cte, cap_page_table_cap_set_capPTMappedObject(cap, 0));

        return EXCEPTION_NONE;
    }

    if (label != RISCVPageTableMap ) {
        userError("RISCVPageTable: Illegal operation.");
        current_syscall_error.type = seL4_IllegalOperation;
        return EXCEPTION_SYSCALL_ERROR;
    }

    vaddr = getSyscallArg(0, buffer) & (~MASK(PT_BITS + PAGE_BITS));
    attr = vmAttributesFromWord(getSyscallArg(1, buffer));
    vspaceCap = extraCaps.excaprefs[0]->cap;

    vspace = (void*)pptr_of_cap(vspaceCap);
    pdSlot = (lookupPDSlot(vspace, vaddr));

    paddr = pptr_to_paddr(PTE_PTR(cap_page_table_cap_get_capPTBasePtr(cap)));

    pde = pde_new(
                      0,  /* ppn1 */
                      0,  /* sw */
                      0,  /* dirty */
                      0,  /* read */
                      0,  /* type */
                      0  /* valid */
                  );

    //cap = cap_page_table_cap_set_capPTMappedObject(cap, PD_REF(pdSlot.pd));
    //cap = cap_page_table_cap_set_capPTMappedIndex(cap, pdSlot.pdIndex);

    cdtUpdate(cte, cap);
    *pdSlot = pde;
    
}

struct create_mappings_pte_return {
    exception_t status;
    pte_t pte;
    pte_range_t pte_entries;
};
typedef struct create_mappings_pte_return create_mappings_pte_return_t;

struct create_mappings_pde_return {
    exception_t status;
    pde_t pde;
    pde_range_t pde_entries;
};
typedef struct create_mappings_pde_return create_mappings_pde_return_t;

static create_mappings_pte_return_t
createSafeMappingEntries_PTE
(paddr_t base, word_t vaddr, vm_page_size_t frameSize,
 vm_rights_t vmRights, vm_attributes_t attr, pde_t *pd)
{
}



static create_mappings_pde_return_t
createSafeMappingEntries_PDE
(paddr_t base, word_t vaddr, vm_page_size_t frameSize,
 vm_rights_t vmRights, vm_attributes_t attr, pde_t *pd)
{
}

static exception_t
decodeRISCVFrameInvocation(word_t label, unsigned int length,
                         cte_t *cte, cap_t cap, extra_caps_t extraCaps,
                         word_t *buffer)
{
}

static const resolve_ret_t default_resolve_ret_t;

static resolve_ret_t
resolveVAddr(pde_t *pd, vptr_t vaddr)
{
}

static inline vptr_t
pageBase(vptr_t vaddr, vm_page_size_t size)
{
    return vaddr & ~MASK(pageBitsForSize(size));
}

static exception_t
decodeRISCVPageDirectoryInvocation
(
    word_t label,
    unsigned int length,
    cte_t* cte,
    cap_t cap,
    extra_caps_t extraCaps,
    word_t* buffer
)
{
    current_syscall_error.type = seL4_IllegalOperation;
    return EXCEPTION_SYSCALL_ERROR;
}

exception_t
decodeRISCVMMUInvocation(word_t label, unsigned int length, cptr_t cptr,
                       cte_t *cte, cap_t cap, extra_caps_t extraCaps,
                       word_t *buffer)
{
    switch (cap_get_capType(cap)) {

    case cap_page_directory_cap:
        return decodeRISCVPageDirectoryInvocation(label, length, cte, cap, extraCaps, buffer);

    case cap_page_table_cap:
        return decodeRISCVPageTableInvocation(label, length, cte, cap, extraCaps, buffer);

    case cap_frame_cap:
        return decodeRISCVFrameInvocation(label, length, cte, cap, extraCaps, buffer);

    default:
        fail("Invalid arch cap type");
    }
}

exception_t
performPageTableInvocationUnmap(cap_t cap, cte_t *ctSlot)
{
}

static exception_t
performPageGetAddress(void *vbase_ptr)
{
}

static bool_t PURE
pteCheckIfMapped(pte_t *pte)
{
}

static bool_t PURE
pdeCheckIfMapped(pde_t *pde)
{
}

exception_t performPageInvocationMapPTE(cap_t cap, cte_t *ctSlot,
                                        pte_t pte, pte_range_t pte_entries)
{
}
exception_t performPageInvocationMapPDE(cap_t cap, cte_t *ctSlot,
                                        pde_t pde, pde_range_t pde_entries)
{
}

exception_t
performPageInvocationRemapPTE(asid_t asid, pte_t pte, pte_range_t pte_entries)
{
    return EXCEPTION_NONE;
}

exception_t
performPageInvocationRemapPDE(asid_t asid, pde_t pde, pde_range_t pde_entries)
{
    return EXCEPTION_NONE;
}

exception_t
performPageInvocationUnmap(cap_t cap, cte_t *ctSlot)
{
    return EXCEPTION_NONE;
}

exception_t
performASIDControlInvocation(void *frame, cte_t *slot,
                             cte_t *parent, asid_t asid_base)
{
    return EXCEPTION_NONE;
}

/*exception_t
performASIDPoolInvocation(asid_t asid, asid_pool_t *poolPtr,
                          cte_t *pdCapSlot)
{
    return EXCEPTION_NONE;
}
*/
void
doFlush(int label, vptr_t start, vptr_t end, paddr_t pstart)
{
}

static exception_t
performPageFlush(int label, pde_t *pd, asid_t asid, vptr_t start,
                 vptr_t end, paddr_t pstart)
{
    return EXCEPTION_NONE;
}

static exception_t
performPDFlush(int label, pde_t *pd, asid_t asid, vptr_t start,
               vptr_t end, paddr_t pstart)
{
    return EXCEPTION_NONE;
}

#ifdef DEBUG
void kernelPrefetchAbort(word_t pc) VISIBLE;
void kernelDataAbort(word_t pc) VISIBLE;

void
kernelPrefetchAbort(word_t pc)
{
}

void
kernelDataAbort(word_t pc)
{
}
#endif
