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

//extern char trap_entry[0];
char riscv_kernel_stack[1024*1024*4] __attribute__ ((aligned(4*1024))) BOOT_DATA;
pde_t l1pt[PTES_PER_PT] __attribute__ ((aligned(4*1024*1024))) BOOT_DATA;
pte_t l2pt[PTES_PER_PT] __attribute__ ((aligned(4*1024*1024))) BOOT_DATA;

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


static word_t CONST
APFromVMRights(vm_rights_t vm_rights)
{
    switch (vm_rights) {
    case VMNoAccess:
        return 0;

    case VMKernelOnly:
        return RISCV_PTE_TYPE_SRWX;

    case VMReadOnly:
        return RISCV_PTE_TYPE_UR_SR;

    case VMReadWrite:
        return RISCV_PTE_TYPE_URWX_SRWX;

    default:
        fail("Invalid VM rights");
    }
}

BOOT_CODE void
map_kernel_frame(paddr_t paddr, pptr_t vaddr, vm_rights_t vm_rights)
{

    /* First level page table */
    uint32_t idx = VIRT0_TO_IDX(vaddr);

    /* vaddr lies in the region the global PT covers */
    assert(vaddr >= PPTR_TOP);

    l2pt[idx] = pte_new(
      VIRT1_TO_IDX(paddr), /* ppn1 */
      VIRT0_TO_IDX(paddr), /* ppn0 */
      0, /* sw */
      0, /* dirty */
      0, /* read */
      APFromVMRights(vm_rights), /* type */
      1 /* valid */
    );   
}

BOOT_CODE VISIBLE void
map_kernel_window(void)
{
    paddr_t  phys;
    uint32_t idx, limit;
    pde_t    pde;
    long     i;

    /* mapping of kernelBase (virtual address) to kernel's physBase  */
    /* up to end of virtual address space minus 4MB */
    phys = VIRT1_TO_IDX(physBase);
    idx  = VIRT1_TO_IDX(kernelBase);
    limit = idx + 63;
     
    /*  4 MB Mega Pages that covers 256 MiB - total memory */
    for(i = 0; idx < limit ; idx++, phys++)
    {
      l1pt[idx] = pde_new(
              phys,
              0,
              0,  /* sw */
              0,  /* dirty */ 
              0,  /* read */
              RISCV_PTE_TYPE_SRWX, /* type */
              1 /* valid */
       );    
    }

    printf("(phys << 22) = %x \n", (phys << 22));
    printf("PADDR_TOP = %x \n", PADDR_TOP);
    assert((phys << 22) == PADDR_TOP);

        /* point to the next last 4MB physical page index */
    //phys++;
    //idx++;

    /* Map last 4MiB Page to page tables - 80400000 */

    uint32_t pt_phys_to_pde = (addrFromPPtr(l2pt)) / 0x1000;

    l1pt[idx] = pde_new(
              pt_phys_to_pde >> 10,
              (0x3ff & pt_phys_to_pde),
              0,  /* sw */
              0,  /* dirty */ 
              0,  /* read */
              RISCV_PTE_TYPE_TABLE, /* type */
              1 /* valid */
       );

    /* now start initialising the page table */
    memzero(l2pt, 1 << 12);
    
    /* map global page */
    map_kernel_frame(
       addrFromPPtr(riscvKSGlobalsFrame),
       PPTR_GLOBALS_PAGE, 
       VMKernelOnly);

    /* Map user<->supervisor system call handler */
    map_kernel_frame(
       addrFromPPtr(trap_entry),
       PPTR_VECTOR_TABLE, 
       VMKernelOnly);
     
    write_csr(stvec, PPTR_VECTOR_TABLE);
    write_csr(sptbr, addrFromPPtr(l1pt));
}

BOOT_CODE void
map_it_pt_cap(cap_t pt_cap)
{
    pde_t* pd   = PDE_PTR(cap_page_table_cap_get_capPTMappedObject(pt_cap));
    pte_t* pt   = PTE_PTR(cap_page_table_cap_get_capPTBasePtr(pt_cap));
    uint32_t pdIndex = cap_page_table_cap_get_capPTMappedIndex(pt_cap);
    pde_t* targetSlot = pd + pdIndex;
    int i = 0;

    uint32_t pt_phys_to_pde = (addrFromPPtr(pt))/ 0x1000;

    *targetSlot = pde_new(
                      pt_phys_to_pde >> 10, /* address */
                      (0x3ff & pt_phys_to_pde), /* address */
                      0, /* sw */
                      0, /* dirty */
                      0, /* read */
                      RISCV_PTE_TYPE_TABLE, /* type */
                      1 /* valid */
                      );
}

BOOT_CODE void
map_it_frame_cap(cap_t frame_cap)
{
    pte_t* pt;
    pte_t* targetSlot;
    uint32_t index;
    void*  frame = (void*)cap_frame_cap_get_capFBasePtr(frame_cap);

    pt = PT_PTR(cap_frame_cap_get_capFMappedObject(frame_cap));
    index = cap_frame_cap_get_capFMappedIndex(frame_cap);
    targetSlot = pt + index;

    *targetSlot = pte_new(
                  VIRT1_TO_IDX((uint32_t)addrFromPPtr(frame)), /* ppn1 */
                  VIRT0_TO_IDX((uint32_t)addrFromPPtr(frame)), /* ppn0 */
                  0, /* sw */
                  0, /* dirty */
                  0, /* read */
                  APFromVMRights(VMReadWrite), /* type */
                  1 /* valid */
                );
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
    unsigned int i;
    pde_t *global_pd = l1pt;

    for (i = VIRT1_TO_IDX(kernelBase); i < BIT(PD_BITS); i++) {
            newPD[i] = global_pd[i];
    }
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
                      0,  /* ppn0 */
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
                      0,  /* ppn0 */
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
