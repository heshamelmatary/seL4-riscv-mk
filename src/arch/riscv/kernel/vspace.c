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
char riscv_kernel_stack[4096] __attribute__ ((aligned(4096))) BOOT_DATA;
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
       VMReadOnly);

    /* map kernel stack */
    map_kernel_frame(
       addrFromPPtr(riscv_kernel_stack),
       PPTR_KERNEL_STACK, 
       VMKernelOnly);

    /* Map user<->supervisor system call handler */
    /*map_kernel_frame(
       addrFromPPtr(trap_entry),
       PPTR_VECTOR_TABLE, 
       VMKernelOnly);
    */
    write_csr(stvec, PPTR_VECTOR_TABLE);
    setCurrentPD(addrFromPPtr(l1pt));
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
    word_t w_bufferPtr;
    cap_t bufferCap;
    vm_rights_t vm_rights;

    w_bufferPtr = thread->tcbIPCBuffer;
    bufferCap = TCB_PTR_CTE_PTR(thread, tcbBuffer)->cap;

    if (unlikely(cap_get_capType(bufferCap) != cap_frame_cap &&
                 cap_get_capType(bufferCap) != cap_frame_cap)) {
        return NULL;
    }

    vm_rights = cap_frame_cap_get_capFVMRights(bufferCap);
    if (likely(vm_rights == VMReadWrite ||
               (!isReceiver && vm_rights == VMReadOnly))) {
        word_t basePtr;
        unsigned int pageBits;

        basePtr = cap_frame_cap_get_capFBasePtr(bufferCap);
        pageBits = pageBitsForSize(cap_frame_cap_get_capFSize(bufferCap));
        return (word_t *)(basePtr + (w_bufferPtr & MASK(pageBits)));
    } else {
        return NULL;
    }
}

/*findPDForASID_ret_t
findPDForASID(asid_t asid)
{
}
*/
lookupPTSlot_ret_t
lookupPTSlot(pde_t *pd, vptr_t vptr)
{
    lookupPTSlot_ret_t ret;
    pde_t *pdSlot;

    pdSlot = pd + VIRT1_TO_IDX(vptr);

    if ( *((uint32_t *) pdSlot) == 0 ) {
        current_lookup_fault = lookup_fault_missing_capability_new(20);

        ret.pt = NULL;
        ret.ptIndex = 0;
        ret.status = EXCEPTION_LOOKUP_FAULT;
        return ret;
    } else {
        pte_t *pt;
        unsigned int ptIndex;
        uint32_t ppn1, ppn0, pt_resolve;

        ppn1 = pde_get_ppn1(*pdSlot);
        ppn0 = pde_get_ppn0(*pdSlot);
        pt_resolve = ppn1 << 10 | ppn0;
        pt_resolve = pt_resolve * 0x1000;

        pte_t *pt_ptr = ptrFromPAddr(pt_resolve);

        ptIndex = VIRT0_TO_IDX(vptr);
        ret.pt = pt_ptr;
        ret.ptIndex = ptIndex;
        ret.status = EXCEPTION_NONE;
        return ret;
    }
}

static pte_t *
lookupPTSlot_nofail(pte_t *pt, vptr_t vptr)
{
  printf("Hit unimplemented lookupPTSlot_nofail \n");
}

pde_t * CONST
lookupPDSlot(pde_t *pd, vptr_t vptr)
{
  printf("Hit unimplemented lookupPDSlot \n");
}

exception_t
handleVMFault(tcb_t *thread, vm_fault_type_t vm_faultType)
{
    printf("Hit unimplemented handleVMFault \n");
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
    printf("Hit unimplemented pageTableMapped \n");
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
    printf("Hit unimplemented pte_pte_invalid_new \n");
}

void
unmapPage(vm_page_size_t page_size, asid_t asid, vptr_t vptr, void *pptr)
{
    printf("Hit unimplemented unmapPage \n");
}

void
setVMRoot(tcb_t *tcb)
{
    cap_t threadRoot;
    //asid_t asid;
    pde_t *pd;
    //findPDForASID_ret_t  find_ret;

    threadRoot = TCB_PTR_CTE_PTR(tcb, tcbVTable)->cap;

    if (cap_get_capType(threadRoot) != cap_page_directory_cap) {
        setCurrentPD(addrFromPPtr(l1pt));
        return;
    }

    pd = PDE_PTR(cap_page_directory_cap_get_capPDBasePtr(threadRoot));

    /* FIXME */
    //asid = 0;
    //asid = cap_page_directory_cap_get_capPDMappedASID(threadRoot);
    /*find_ret = findPDForASID(asid);
    if (unlikely(find_ret.status != EXCEPTION_NONE || find_ret.pd != pd)) {
        setCurrentPD(addrFromPPtr(l1pt));
        return;
    }*/

    riscv_vm_contextSwitch(pd);
}

static bool_t
setVMRootForFlush(pde_t* pd, asid_t asid)
{
    printf("Hit unimplemented setVMRootForFlush \n");
}

bool_t CONST
isValidVTableRoot(cap_t cap)
{
    return cap_get_capType(cap) == cap_page_directory_cap;
}

exception_t
checkValidIPCBuffer(vptr_t vptr, cap_t cap)
{
    if (unlikely(cap_get_capType(cap) != cap_frame_cap)) {
        userError("Requested IPC Buffer is not a frame cap."); 
        current_syscall_error.type = seL4_IllegalOperation;
        return EXCEPTION_SYSCALL_ERROR;
    }

    if (unlikely(vptr & MASK(9))) {
        userError("Requested IPC Buffer location 0x%x is not aligned.",
                  (int)vptr);
        current_syscall_error.type = seL4_AlignmentError;
        return EXCEPTION_SYSCALL_ERROR;
    }

    return EXCEPTION_NONE;
}

vm_rights_t CONST
maskVMRights(vm_rights_t vm_rights, cap_rights_t cap_rights_mask)
{
    if (vm_rights == VMNoAccess) {
        return VMNoAccess;
    }
    if (vm_rights == VMReadOnly &&
            cap_rights_get_capAllowRead(cap_rights_mask)) {
        return VMReadOnly;
    }
    if (vm_rights == VMReadWrite &&
            cap_rights_get_capAllowRead(cap_rights_mask)) {
        if (!cap_rights_get_capAllowWrite(cap_rights_mask)) {
            return VMReadOnly;
        } else {
            return VMReadWrite;
        }
    }
    return VMKernelOnly;
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
    printf("Hit unimplemented flushPage \n");
}

 void flushTable(pde_t* pd, word_t vptr, pte_t* pt)
{
    printf("Hit unimplemented flushTable \n");
}

void
flushSpace(asid_t asid)
{
    printf("Hit unimplemented flushSpace \n");
}

void
invalidateTLBByASID(asid_t asid)
{
}

/* The rest of the file implements the RISCV object invocations */

static pte_t CONST
makeUserPTE(vm_page_size_t page_size, paddr_t paddr, vm_rights_t vm_rights)
{
    pte_t pte; 

    switch (page_size) {
    case RISCVNormalPage: {
 
         pte = pte_new(
                  VIRT1_TO_IDX((uint32_t)addrFromPPtr(paddr)), /* ppn1 */
                  VIRT0_TO_IDX((uint32_t)addrFromPPtr(paddr)), /* ppn0 */
                  0, /* sw */
                  0, /* dirty */
                  0, /* read */
                  APFromVMRights(vm_rights), /* type */
                  1 /* valid */
                );
        break;
    }
    default:
        fail("Invalid PTE frame type");
    }    

    return pte;
}

static pde_t CONST
makeUserPDE(vm_page_size_t page_size, paddr_t paddr, bool_t parity,
            bool_t cacheable, bool_t nonexecutable, word_t domain,
            vm_rights_t vm_rights)
{
    printf("Hit unimplemented makeUserPDE \n");
}

static inline bool_t CONST
checkVPAlignment(vm_page_size_t sz, word_t w)
{
    return (w & MASK(pageBitsForSize(sz))) == 0;
}

static exception_t
decodeRISCVPageTableInvocation(word_t label, unsigned int length,
                             cte_t *cte, cap_t cap, extra_caps_t extraCaps,
                             word_t *buffer)
{
    word_t vaddr, pdIndex;
    vm_attributes_t attr;
    cap_t pdCap;
    pde_t *pd, *pdSlot;
    pde_t pde;
    paddr_t paddr;

    if (label == RISCVPageTableUnmap) {
        setThreadState(ksCurThread, ThreadState_Restart);
        return performPageTableInvocationUnmap (cap, cte);
    }
    
    if (unlikely(label != RISCVPageTableMap)) {
        current_syscall_error.type = seL4_IllegalOperation;
        return EXCEPTION_SYSCALL_ERROR;
    }
    
    if (unlikely(length < 2 || extraCaps.excaprefs[0] == NULL)) {
        current_syscall_error.type = seL4_TruncatedMessage;
        return EXCEPTION_SYSCALL_ERROR;
    }
    
    vaddr = getSyscallArg(0, buffer);
    attr = vmAttributesFromWord(getSyscallArg(1, buffer));
    pdCap = extraCaps.excaprefs[0]->cap;
    
    if (unlikely(cap_get_capType(pdCap) != cap_page_directory_cap)) {
        current_syscall_error.type = seL4_InvalidCapability;
        current_syscall_error.invalidCapNumber = 1;

        return EXCEPTION_SYSCALL_ERROR;
    }

    pd = PDE_PTR(cap_page_directory_cap_get_capPDBasePtr(pdCap));

    if (unlikely(vaddr >= kernelBase)) {
        current_syscall_error.type = seL4_InvalidArgument;
        current_syscall_error.invalidArgumentNumber = 0;

        return EXCEPTION_SYSCALL_ERROR;
    }

    pdIndex = vaddr >> 22;

    pdSlot = &pd[pdIndex];

    if (unlikely( *((uint32_t *) pdSlot) != 0) ) {
        current_syscall_error.type = seL4_DeleteFirst;

        return EXCEPTION_SYSCALL_ERROR;
    }

    paddr = addrFromPPtr(
                PTE_PTR(cap_page_table_cap_get_capPTBasePtr(cap)));

    uint32_t pt_phys_to_pde = paddr / 0x1000;

    pde = pde_new(
                      pt_phys_to_pde >> 10, /* address */
                      (0x3ff & pt_phys_to_pde), /* address */
                      0, /* sw */
                      0, /* dirty */
                      0, /* read */
                      RISCV_PTE_TYPE_TABLE, /* type */
                      1 /* valid */
                      );

    cap = cap_page_table_cap_set_capPTMappedObject(cap, PD_REF(pd));
    cap = cap_page_table_cap_set_capPTMappedIndex(cap, pdIndex);

    setThreadState(ksCurThread, ThreadState_Restart);
    return performPageTableInvocationMap(cap, cte, pde, pdSlot);
    
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
    create_mappings_pte_return_t ret;
    lookupPTSlot_ret_t lu_ret;
    unsigned int i;

    switch (frameSize) {

    case RISCVNormalPage:

        ret.pte_entries.pt = NULL; /* to avoid uninitialised warning */
        ret.pte_entries.start = 0;
        ret.pte_entries.length = 1;

        ret.pte = makeUserPTE(RISCVNormalPage, base, vmRights);

        lu_ret = lookupPTSlot(pd, vaddr);
        if (unlikely(lu_ret.status != EXCEPTION_NONE)) {
            current_syscall_error.type =
                seL4_FailedLookup;
            current_syscall_error.failedLookupWasSource =
                false;
            ret.status = EXCEPTION_SYSCALL_ERROR;
            /* current_lookup_fault will have been set by
             * lookupPTSlot */
            return ret;
        }

        ret.pte_entries.pt = lu_ret.pt;
        ret.pte_entries.start = lu_ret.ptIndex;

        ret.status = EXCEPTION_NONE;
        return ret; 

    default:
        fail("Invalid or unexpected ARM page type.");

    }
}



static create_mappings_pde_return_t
createSafeMappingEntries_PDE
(paddr_t base, word_t vaddr, vm_page_size_t frameSize,
 vm_rights_t vmRights, vm_attributes_t attr, pde_t *pd)
{
    printf("Hit unimplemented createSafeMappingEntries_PDE \n");
}

static exception_t
decodeRISCVFrameInvocation(word_t label, unsigned int length,
                         cte_t *cte, cap_t cap, extra_caps_t extraCaps,
                         word_t *buffer)
{
    switch (label) {
    case RISCVPageMap: {

        word_t vaddr, vtop, w_rightsMask;
        paddr_t capFBasePtr;
        cap_t pdCap;
        pde_t *pd;
        asid_t asid;
        vm_rights_t capVMRights, vmRights;
        vm_page_size_t frameSize;
        vm_attributes_t attr;

        if (unlikely(length < 3 || extraCaps.excaprefs[0] == NULL)) {
            current_syscall_error.type =
                seL4_TruncatedMessage;

            return EXCEPTION_SYSCALL_ERROR;
        }

        vaddr = getSyscallArg(0, buffer);

        w_rightsMask = getSyscallArg(1, buffer);
        attr = vmAttributesFromWord(getSyscallArg(2, buffer));
        pdCap = extraCaps.excaprefs[0]->cap;

        frameSize = cap_frame_cap_get_capFSize(cap);
        capVMRights = cap_frame_cap_get_capFVMRights(cap);

        if (unlikely(cap_frame_cap_get_capFMappedObject(cap))) {
            current_syscall_error.type =
                seL4_InvalidCapability;
            current_syscall_error.invalidCapNumber = 0;

            return EXCEPTION_SYSCALL_ERROR;
        }

        if (unlikely(cap_get_capType(pdCap) != cap_page_directory_cap)) {
            current_syscall_error.type =
                seL4_InvalidCapability;
            current_syscall_error.invalidCapNumber = 1;

            return EXCEPTION_SYSCALL_ERROR;
        }
        pd = PDE_PTR(cap_page_directory_cap_get_capPDBasePtr(
                         pdCap));
        /*asid = cap_page_directory_cap_get_capPDMappedASID(pdCap);

        {
            findPDForASID_ret_t find_ret;

            find_ret = findPDForASID(asid);
            if (unlikely(find_ret.status != EXCEPTION_NONE)) {
                userError("ARMPageMap: No PD for ASID");
                current_syscall_error.type =
                    seL4_FailedLookup;
                current_syscall_error.failedLookupWasSource =
                    false;

                return EXCEPTION_SYSCALL_ERROR;
            }

            if (unlikely(find_ret.pd != pd)) {
                current_syscall_error.type =
                    seL4_InvalidCapability;
                current_syscall_error.invalidCapNumber = 1;

                return EXCEPTION_SYSCALL_ERROR;
            }
        }*/

        vtop = vaddr + BIT(pageBitsForSize(frameSize)) - 1;

        if (unlikely(vtop >= kernelBase)) {
            current_syscall_error.type =
                seL4_InvalidArgument;
            current_syscall_error.invalidArgumentNumber = 0;

            return EXCEPTION_SYSCALL_ERROR;
        }

        vmRights =
            maskVMRights(capVMRights, rightsFromWord(w_rightsMask));

        if (unlikely(!checkVPAlignment(frameSize, vaddr))) {
            current_syscall_error.type =
                seL4_AlignmentError;

            return EXCEPTION_SYSCALL_ERROR;
        }

        capFBasePtr = addrFromPPtr((void *)
                                   cap_frame_cap_get_capFBasePtr(cap));

        asid = 0;
        //cap = cap_frame_cap_set_capFMappedAddress(cap, asid, vaddr);
        if (frameSize == RISCVNormalPage) {
            create_mappings_pte_return_t map_ret;
            map_ret = createSafeMappingEntries_PTE(capFBasePtr, vaddr,
                                                   frameSize, vmRights,
                                                   attr, pd);

            if (unlikely(map_ret.status != EXCEPTION_NONE)) {
                return map_ret.status;
            }

            setThreadState(ksCurThread, ThreadState_Restart);
            return performPageInvocationMapPTE(cap, cte,
                                               map_ret.pte,
                                               map_ret.pte_entries);
        } else {
            printf("error: Not a valid page size \n");
        }
     }
  }
    
}

static const resolve_ret_t default_resolve_ret_t;

static resolve_ret_t
resolveVAddr(pde_t *pd, vptr_t vaddr)
{
    printf("Hit unimplemented resolveVAddr \n");
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
    printf("Hit unimplemented decodeRISCVPageDirectoryInvocation \n");
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
performPageTableInvocationMap(cap_t cap, cte_t *ctSlot,
                              pde_t pde, pde_t *pdSlot)
{
    ctSlot->cap = cap;
    *pdSlot = pde;

    return EXCEPTION_NONE;
}

exception_t
performPageTableInvocationUnmap(cap_t cap, cte_t *ctSlot)
{
    printf("Hit unimplemented performPageTableInvocationUnmap \n");
}

static exception_t
performPageGetAddress(void *vbase_ptr)
{
    printf("Hit unimplemented performPageGetAddress \n");
}

static bool_t PURE
pteCheckIfMapped(pte_t *pte)
{
    printf("Hit unimplemented pteCheckIfMapped \n");
}

static bool_t PURE
pdeCheckIfMapped(pde_t *pde)
{
    printf("Hit unimplemented pdeCheckIfMapped \n");
}

exception_t performPageInvocationMapPTE(cap_t cap, cte_t *ctSlot,
                                        pte_t pte, pte_range_t pte_entries)
{
    unsigned int i;

    cap = cap_frame_cap_set_capFMappedObject(cap, PT_REF(pte_entries.pt));
    cap = cap_frame_cap_set_capFMappedIndex(cap, pte_entries.start);
    cdtUpdate(ctSlot, cap);

    for (i = 0; i < pte_entries.length; i++) {
        pte_entries.pt[pte_entries.start + i] = pte;
    }
    return EXCEPTION_NONE;
}
exception_t performPageInvocationMapPDE(cap_t cap, cte_t *ctSlot,
                                        pde_t pde, pde_range_t pde_entries)
{
    printf("Hit unimplemented performPageInvocationMapPDE \n");
}

exception_t
performPageInvocationRemapPTE(asid_t asid, pte_t pte, pte_range_t pte_entries)
{
    printf("Hit unimplemented performPageInvocationRemapPTE \n");
    return EXCEPTION_NONE;
}

exception_t
performPageInvocationRemapPDE(asid_t asid, pde_t pde, pde_range_t pde_entries)
{
    printf("Hit unimplemented performPageInvocationRemapPDE \n");
    return EXCEPTION_NONE;
}

exception_t
performPageInvocationUnmap(cap_t cap, cte_t *ctSlot)
{
    printf("Hit unimplemented performPageInvocationUnmap \n");
    return EXCEPTION_NONE;
}

exception_t
performASIDControlInvocation(void *frame, cte_t *slot,
                             cte_t *parent, asid_t asid_base)
{
    printf("Hit unimplemented performASIDControlInvocation \n");
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
    printf("Hit unimplemented doFlush \n");
}

static exception_t
performPageFlush(int label, pde_t *pd, asid_t asid, vptr_t start,
                 vptr_t end, paddr_t pstart)
{
    printf("Hit unimplemented performPageFlush \n");
    return EXCEPTION_NONE;
}

static exception_t
performPDFlush(int label, pde_t *pd, asid_t asid, vptr_t start,
               vptr_t end, paddr_t pstart)
{
    printf("Hit unimplemented performPDFlush \n");
    return EXCEPTION_NONE;
}

#ifdef DEBUG
void kernelPrefetchAbort(word_t pc) VISIBLE;
void kernelDataAbort(word_t pc) VISIBLE;

void
kernelPrefetchAbort(word_t pc)
{
    printf("Hit unimplemented kernelPrefetchAbort \n");
}

void
kernelDataAbort(word_t pc)
{
    printf("Hit unimplemented kernelDataAbort \n");
}
#endif
