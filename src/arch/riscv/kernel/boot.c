/*
 * Copyright 2015 Hesham Almatary <heshamelmatary@gmail.com>
 * Author: Hesham Almatary <heshamelmatary@gmail.com>
 */

/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <assert.h>
#include <kernel/boot.h>
#include <machine/io.h>
#include <model/statedata.h>
#include <object/interrupt.h>
#include <arch/machine.h>
#include <arch/kernel/boot.h>
#include <arch/kernel/vspace.h>
#include <arch/benchmark.h>
//#include <arch/user_access.h>
#include <arch/linker.h>
#include <plat/machine/hardware.h>
#include <machine.h>
#include <limits.h>
#include <stdarg.h>

//#include <string.h>

void trap_entry();
void pop_tf(trapframe_t*);

typedef struct { pte_tt addr; void* next; } freelist_t;

char test_area[4096] __attribute__((aligned(4*1024*1024))) BOOT_DATA;
void *kernel_elf;

/* pointer to the end of boot code/data in kernel image */
/* need a fake array to get the pointer from the linker script */
extern char ki_boot_end[1];
/* pointer to end of kernel image */
extern char ki_end[1];

BOOT_CODE cap_t
create_unmapped_it_frame_cap(pptr_t pptr, bool_t use_large)
{
}

BOOT_CODE cap_t
create_mapped_it_frame_cap(cap_t pd_cap, pptr_t pptr, vptr_t vptr, bool_t
use_large, bool_t executable)
{
    cap_t cap;
    pde_t *pd = PD_PTR(cap_page_directory_cap_get_capPDBasePtr(pd_cap));
    pte_t *pt;

#ifdef CONFIG_ROCKET_CHIP
    uint32_t pd_index = SV39_VIRT_TO_VPN1(vptr) & 0x1FF;
    uint32_t pt_index = SV39_VIRT_TO_VPN0(vptr) & 0x1FF;
    uint32_t ppn2, ppn1, ppn0, pt_resolve;
    uint32_t temp; 

    ppn2 = pde_get_ppn2(pd[pd_index]);
    ppn1 = pde_get_ppn1(pd[pd_index]);
    ppn0 = pde_get_ppn0(pd[pd_index]);

    pt_resolve = ppn2 << 18 | ppn1 << 9 | ppn0;
    pt_resolve = pt_resolve * 0x1000;

    pt = ptrFromPAddr(pt_resolve);
    temp = 0x000FFFFF;

    cap = cap_frame_cap_new(
                  FMAPPED_OBJECT_HIGH(PT_REF(pt)) & temp, /* capFMappedObjectHigh */
                  pt_index,                        /* capFMappedIndex      */
                  0,                    /* capFSize             */
                  wordFromVMRights(VMReadWrite),   /* capFVMRights         */
                  FMAPPED_OBJECT_LOW(PT_REF(pt)),  /* capFMappedObjectLow  */
                  pptr                             /* capFBasePtr          */
              );
#else
    uint32_t pd_index = VIRT1_TO_IDX(vptr);
    uint32_t pt_index = VIRT0_TO_IDX(vptr);
    uint32_t ppn1, ppn0, pt_resolve;

    ppn1 = pde_get_ppn1(pd[pd_index]);
    ppn0 = pde_get_ppn0(pd[pd_index]);

    pt_resolve = ppn1 << 10 | ppn0;
    pt_resolve = pt_resolve * 0x1000;

    pt = ptrFromPAddr(pt_resolve);

    cap = cap_frame_cap_new(
                  FMAPPED_OBJECT_HIGH(PT_REF(pt)), /* capFMappedObjectHigh */
                  pt_index,                        /* capFMappedIndex      */
                  0,                    /* capFSize             */
                  wordFromVMRights(VMReadWrite),   /* capFVMRights         */
                  FMAPPED_OBJECT_LOW(PT_REF(pt)),  /* capFMappedObjectLow  */
                  pptr                             /* capFBasePtr          */
              );
#endif
    map_it_frame_cap(cap);
    return cap;
}

BOOT_CODE cap_t
create_mapped_it__bare_frame_cap(cap_t pd_cap, pptr_t pptr, vptr_t vptr, bool_t
use_large, bool_t executable)
{
    cap_t cap;
    pde_t *pd = PD_PTR(cap_page_directory_cap_get_capPDBasePtr(pd_cap));
    pte_t *pt;
    uint32_t pd_index = VIRT1_TO_IDX(vptr);
    uint32_t pt_index = VIRT0_TO_IDX(vptr);
    uint32_t ppn1, ppn0, pt_resolve;

    ppn1 = pde_get_ppn1(pd[pd_index]);
    ppn0 = pde_get_ppn0(pd[pd_index]);

    pt_resolve = ppn1 << 10 | ppn0;
    pt_resolve = pt_resolve * 0x1000;

    pt = ptrFromPAddr(pt_resolve);

    cap = cap_frame_cap_new(
                  FMAPPED_OBJECT_HIGH(PT_REF(pt)), /* capFMappedObjectHigh */
                  pt_index,                        /* capFMappedIndex      */
                  0,                    /* capFSize             */
                  wordFromVMRights(VMReadWrite),   /* capFVMRights         */
                  FMAPPED_OBJECT_LOW(PT_REF(pt)),  /* capFMappedObjectLow  */
                  pptr                             /* capFBasePtr          */
              );

    map_it_bare_frame_cap(cap);
    return cap;
}
/* Create a page table for the initial thread */

static BOOT_CODE cap_t
create_it_page_table_cap(cap_t pd, pptr_t pptr, vptr_t vptr)
{

    cap_t cap;
#ifdef CONFIG_ROCKET_CHIP
    uint32_t pd_index = (SV39_VIRT_TO_VPN1(vptr)) & 0x1FF;
#else
    uint32_t pd_index = VIRT1_TO_IDX(vptr);
#endif
    cap = cap_page_table_cap_new(
              cap_page_directory_cap_get_capPDBasePtr(pd), /* capPTMappedObject */
              pd_index,                                    /* capPTMappedIndex  */
              pptr                                         /* capPTBasePtr      */
          );

    map_it_pt_cap(cap);
    return cap;
}

/* Create an address space for the initial thread.
 * This includes page directory and page tables */
BOOT_CODE static cap_t
create_it_address_space(cap_t root_cnode_cap, v_region_t it_v_reg)
{
    cap_t      pd_cap;
    vptr_t     pt_vptr;
    pptr_t     pt_pptr;
    slot_pos_t slot_pos_before;
    slot_pos_t slot_pos_after;
    pptr_t pd_pptr;

    /* create PD obj and cap */
    pd_pptr = alloc_region(PD_SIZE_BITS);

    if (!pd_pptr) {
        return cap_null_cap_new();
    }
    memzero(PDE_PTR(pd_pptr), 1 << PD_SIZE_BITS);

    copyGlobalMappings(PDE_PTR(pd_pptr));
    
    pd_cap =
        cap_page_directory_cap_new(
            pd_pptr  /* capPDBasePtr    */
        );
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), BI_CAP_IT_VSPACE), pd_cap);

    /* create all PT objs and caps necessary to cover userland image */
    slot_pos_before = ndks_boot.slot_pos_cur;

    for (pt_vptr = ROUND_DOWN(it_v_reg.start, PT_BITS + PAGE_BITS);
            pt_vptr < it_v_reg.end;
            pt_vptr += BIT(PT_BITS + PAGE_BITS)) {
        pt_pptr = alloc_region(PT_SIZE_BITS);

        if (!pt_pptr) {
            return cap_null_cap_new();
        }

        memzero(PTE_PTR(pt_pptr), 1 << PT_SIZE_BITS);
        if (!provide_cap(root_cnode_cap,
                         create_it_page_table_cap(pd_cap, pt_pptr, pt_vptr))
           ) {
            return cap_null_cap_new();
        }
    }
    
    /* Allocate PT for kernel in cpio */
    pt_pptr = alloc_region(PT_SIZE_BITS);
    memzero(PTE_PTR(pt_pptr), 1 << PT_SIZE_BITS);
    create_it_page_table_cap(pd_cap, pt_pptr, 0x10000000);

    /* Allocate PT for shared frame */
    pt_pptr = alloc_region(PT_SIZE_BITS);
    memzero(PTE_PTR(pt_pptr), 1 << PT_SIZE_BITS);
    create_it_page_table_cap(pd_cap, pt_pptr, 0xf0000000);

    slot_pos_after = ndks_boot.slot_pos_cur;
    ndks_boot.bi_frame->ui_pt_caps = (slot_region_t) {
        slot_pos_before, slot_pos_after
    };

    setCurrentPD(addrFromPPtr(pd_pptr));
    return pd_cap;
}


/**
 * Split mem_reg about reserved_reg. If memory exists in the lower
 * segment, insert it. If memory exists in the upper segment, return it.
 */
BOOT_CODE static region_t
insert_region_excluded(region_t mem_reg, region_t reserved_reg)
{
  region_t residual_reg = mem_reg;
    bool_t result UNUSED;

    if (reserved_reg.start < mem_reg.start) {
        /* Reserved region is below the provided mem_reg. */
        mem_reg.end = 0;
        mem_reg.start = 0;
        /* Fit the residual around the reserved region */
        if (reserved_reg.end > residual_reg.start) {
            residual_reg.start = reserved_reg.end;
        }
    } else if (mem_reg.end > reserved_reg.start) {
        /* Split mem_reg around reserved_reg */
        mem_reg.end = reserved_reg.start;
        residual_reg.start = reserved_reg.end;
    } else {
        /* reserved_reg is completely above mem_reg */
        residual_reg.start = 0;
        residual_reg.end = 0;
    }
    /* Add the lower region if it exists */
    if (mem_reg.start < mem_reg.end) {
        result = insert_region(mem_reg);
        assert(result);
    }
    /* Validate the upper region */
    if (residual_reg.start > residual_reg.end) {
        residual_reg.start = residual_reg.end;
    }

    return residual_reg;
}

BOOT_CODE static void
init_freemem(region_t ui_reg)
{
    unsigned int i;
    bool_t result UNUSED;
    region_t cur_reg;
    region_t res_reg[] = {
        {
            .start = kernelBase,
            .end   = (pptr_t)ki_end
        },
        {
            .start = ui_reg.start,
            .end = ui_reg.end
        }
    };

    for (i = 0; i < MAX_NUM_FREEMEM_REG; i++) {
        ndks_boot.freemem[i] = REG_EMPTY;
    }

    /* Force ordering and exclusivity of reserved regions. */
    assert(res_reg[0].start < res_reg[0].end);
    assert(res_reg[1].start < res_reg[1].end);

    assert(res_reg[0].end <= res_reg[1].start);

    for (i = 0; i < get_num_avail_p_regs(); i++) {
        cur_reg = paddr_to_pptr_reg(get_avail_p_reg(i));
        /* Adjust region if it exceeds the kernel window
         * Note that we compare physical address in case of overflow.
         */
        if (pptr_to_paddr((void*)cur_reg.end) > PADDR_TOP) {
            cur_reg.end = PPTR_TOP;
        }
        if (pptr_to_paddr((void*)cur_reg.start) > PADDR_TOP) {
            cur_reg.start = PPTR_TOP;
        }

        cur_reg = insert_region_excluded(cur_reg, res_reg[0]);
        cur_reg = insert_region_excluded(cur_reg, res_reg[1]);

        if (cur_reg.start != cur_reg.end) {
            result = insert_region(cur_reg);
            assert(result);
        }
    }
}

BOOT_CODE static void
init_irqs(cap_t root_cnode_cap)
{
    irq_t i;

    for (i = 0; i <= maxIRQ; i++) {
        setIRQState(IRQInactive, i);
    }
    setIRQState(IRQTimer, KERNEL_TIMER_IRQ);

    /* provide the IRQ control cap */
    write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), BI_CAP_IRQ_CTRL), cap_irq_control_cap_new());
}

BOOT_CODE static bool_t
create_device_frames(cap_t root_cnode_cap)
{
    return true;
}

/* This and only this function initialises the CPU. It does NOT initialise any kernel state. */

BOOT_CODE static void
init_cpu(void)
{
}

/* This and only this function initialises the platform. It does NOT initialise any kernel state. */

BOOT_CODE static void
init_plat(void)
{
    initIRQController();
    initTimer();
    initL2Cache();
}

/* Main kernel initialisation function. */


static BOOT_CODE bool_t
try_init_kernel(
    paddr_t ui_p_reg_start,
    paddr_t ui_p_reg_end,
    int32_t pv_offset,
    vptr_t  v_entry
)
{
    /* kernel successfully initialized */

    printf("Bootstrapping kernel\n");

    cap_t root_cnode_cap;
    cap_t it_pd_cap;
    cap_t ipcbuf_cap;
    region_t ui_reg = paddr_to_pptr_reg((p_region_t) {
        ui_p_reg_start, ui_p_reg_end
    });
    pptr_t bi_frame_pptr;
    vptr_t bi_frame_vptr;
    vptr_t ipcbuf_vptr;
    create_frames_of_region_ret_t create_frames_ret;
    
    /* convert from physical addresses to userland vptrs */
    v_region_t ui_v_reg;
    v_region_t it_v_reg;
    ui_v_reg.start = ui_p_reg_start - pv_offset;
    ui_v_reg.end   = ui_p_reg_end   - pv_offset;

    ipcbuf_vptr = ui_v_reg.end;
    bi_frame_vptr = ipcbuf_vptr + BIT(PAGE_BITS);

    /* The region of the initial thread is the user image + ipcbuf and boot info */
    it_v_reg.start = ui_v_reg.start;
    it_v_reg.end = bi_frame_vptr + BIT(PAGE_BITS);

    map_kernel_window();

    /* initialise the CPU */
    init_cpu();

    /* make the free memory available to alloc_region() */
    init_freemem(ui_reg);

  /* create the root cnode */
    root_cnode_cap = create_root_cnode();
    if (cap_get_capType(root_cnode_cap) == cap_null_cap) {
        return false;
    }

  /* create the cap for managing thread domains */
    create_domain_cap(root_cnode_cap);

      /* create the IRQ CNode */
    if (!create_irq_cnode()) {
        return false;
    }

    /* initialise the IRQ states and provide the IRQ control cap */
    init_irqs(root_cnode_cap);
  
    /* create the bootinfo frame */
    bi_frame_pptr = allocate_bi_frame(0, 1, ipcbuf_vptr);
    if (!bi_frame_pptr) {
        return false;
    }

    printf("Construct an initial address space\n");
    /* Construct an initial address space with enough virtual addresses
     * to cover the user image + ipc buffer and bootinfo frames */
    it_pd_cap = create_it_address_space(root_cnode_cap, it_v_reg);
    if (cap_get_capType(it_pd_cap) == cap_null_cap) {
        printf("cap == null \n");
        return false;
    }

    printf("Create and map bootinfo frame cap \n");
    /* Create and map bootinfo frame cap */
    create_bi_frame_cap(
        root_cnode_cap,
        it_pd_cap,
        bi_frame_pptr,
        bi_frame_vptr
    );

    printf("Create the initial thread's IPC buffer \n");
    /* create the initial thread's IPC buffer */
    ipcbuf_cap = create_ipcbuf_frame(root_cnode_cap, it_pd_cap, ipcbuf_vptr);
    if (cap_get_capType(ipcbuf_cap) == cap_null_cap) {
        return false;
    }

    /* create all userland image frames */
    create_frames_ret =
        create_frames_of_region(
            root_cnode_cap,
            it_pd_cap,
            ui_reg,
            true,
            pv_offset
        );
    if (!create_frames_ret.success) {
        return false;
    }

    ndks_boot.bi_frame->ui_frame_caps = create_frames_ret.region;

    /* Create frame for elf kernel image in cpio 
    for(int i = 0; i < 1024; i++)
    {
      if( cap_get_capType(create_mapped_it__bare_frame_cap(it_pd_cap, 0x4000 + i* 0x1000, 0x10000000 + i* 0x1000, false, false)) == cap_null_cap)
        return false;
    } */
  
    /* Shared frame */
    if( cap_get_capType(create_mapped_it__bare_frame_cap(it_pd_cap, 0xf0000000, 0xf0000000, false, false)) == cap_null_cap)
        return false;

    if( cap_get_capType(create_mapped_it__bare_frame_cap(it_pd_cap, 0xf0001000, 0xf0001000, false, false)) == cap_null_cap)
        return false;

    if (!create_frames_ret.success) {
        return false;
    }

    /* create the idle thread */
    if (!create_idle_thread()) {
        return false;
    }

    /* create the initial thread */
    if (!create_initial_thread(
                root_cnode_cap,
                it_pd_cap,
                v_entry,
                bi_frame_vptr,
                ipcbuf_vptr,
                ipcbuf_cap
            )) {
        return false;
    }

    /* convert the remaining free memory into UT objects and provide the caps */
    if (!create_untypeds(
                root_cnode_cap,
    (region_t) {
    kernelBase, (pptr_t)ki_boot_end
    } /* reusable boot code/data */
            )) {
        return false;
    }

    /* no shared-frame caps (RISCV has no multikernel support) */
    ndks_boot.bi_frame->sh_frame_caps = S_REG_EMPTY;

    /* finalise the bootinfo frame */
    bi_finalise();

  return true;
}

uint32_t blt_test(uint32_t x)
{
  while (!(x & 0x80000000U));
}

/* FIXME: The following is a dirty hack to get over the undefined reference to the 
 &  correspoding libgcc reference, need to figure out why they are not linked
 */
uint32_t __clzsi2(uint32_t x)
{
  uint32_t count = 0;
  while ( !(x & 0x80000000U) && count < 33)
  {
    x <<= 1;
    count++;
  }
  return count;
}
   
uint32_t __ctzsi2(uint32_t x)
{
  uint32_t count = 0;
  while ( !(x & 0x000000001) && count <= 32)
  {
    x >>= 1;
    count++;
  }
  return count;
}

typedef void (*user_entry_t)(void);

BOOT_CODE VISIBLE void
init_kernel(
    paddr_t ui_p_reg_start,
    paddr_t ui_p_reg_end,
    int32_t pv_offset,
    vptr_t  v_entry
)
{
    printf( "********* seL4 microkernel on RISC-V 32-bit platform *********\n"); 

    //init_plat();
    bool_t result;
  
    result = try_init_kernel(ui_p_reg_start,
                             ui_p_reg_end,
                             pv_offset,
                             v_entry);
        
    if (!result) {
        fail ("Kernel init failed for some reason :(");
    }

    /* Enable floating point unit */
    set_csr(sstatus, SSTATUS_FS);
    /* Set to user mode */
    clear_csr(sstatus, 0x10);
    write_csr(sepc, v_entry);
    /* Set vector table address for S-Mode */
    write_csr(stvec, PPTR_VECTOR_TABLE);
    write_csr(sscratch, ksCurThread);
}

