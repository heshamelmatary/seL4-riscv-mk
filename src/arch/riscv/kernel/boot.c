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
//#include <string.h>

void trap_entry();
void pop_tf(trapframe_t*);

static void cputchar(int x)
{
  while (swap_csr(tohost, 0x0101000000000000 | (unsigned char)x));
  while (swap_csr(fromhost, 0) == 0);
}

static void cputstring(const char* s)
{
  while(*s)
    cputchar(*s++);
}

static void terminate(int code)
{
  while (swap_csr(tohost, code));
  while (1);
}

#define stringify1(x) #x
#define stringify(x) stringify1(x)
#define assert(x) do { \
  if (x) break; \
  cputstring("Assertion failed: " stringify(x) "\n"); \
  terminate(3); \
} while(0)

typedef struct { pte_tt addr; void* next; } freelist_t;

pte_tt l1pt[PTES_PER_PT] __attribute__((aligned(PGSIZE)));
pte_tt l2pt[PTES_PER_PT] __attribute__((aligned(PGSIZE)));
pte_tt l3pt[PTES_PER_PT] __attribute__((aligned(PGSIZE)));
freelist_t user_mapping[MAX_TEST_PAGES];
freelist_t freelist_nodes[MAX_TEST_PAGES];
freelist_t *freelist_head, *freelist_tail;

/* pointer to the end of boot code/data in kernel image */
/* need a fake array to get the pointer from the linker script */
extern char ki_boot_end[1];
/* pointer to end of kernel image */
extern char ki_end[1];

/**
 * Split mem_reg about reserved_reg. If memory exists in the lower
 * segment, insert it. If memory exists in the upper segment, return it.
 */
BOOT_CODE static region_t
insert_region_excluded(region_t mem_reg, region_t reserved_reg)
{
}

BOOT_CODE static void
init_freemem(region_t ui_reg)
{
}

BOOT_CODE static void
init_irqs(cap_t root_cnode_cap)
{
}

/* Create a frame cap for the initial thread. */

static BOOT_CODE cap_t
create_it_frame_cap(pptr_t pptr, vptr_t vptr, asid_t asid, bool_t use_large)
{
}

BOOT_CODE cap_t
create_unmapped_it_frame_cap(pptr_t pptr, bool_t use_large)
{
}

BOOT_CODE cap_t
create_mapped_it_frame_cap(cap_t pd_cap, pptr_t pptr, vptr_t vptr, bool_t
use_large, bool_t executable)
{
}

/* Create a page table for the initial thread */

static BOOT_CODE cap_t
create_it_page_table_cap(cap_t pd, pptr_t pptr, vptr_t vptr, asid_t asid)
{
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
    map_kernel_window();
    
    // page directory
    return true;
}

void vm_boot(long test_addr, long seed)
{
  while (read_csr(hartid) > 0); // only core 0 proceeds
  long i = 0;
  //assert(SIZEOF_TRAPFRAME_T == sizeof(trapframe_t));

  l1pt[0] = ((pte_tt)l2pt >> PGSHIFT << PTE_PPN_SHIFT) | PTE_TYPE_TABLE;
  l2pt[0] = ((pte_tt)l3pt >> PGSHIFT << PTE_PPN_SHIFT) | PTE_TYPE_TABLE;
  write_csr(sptbr, l1pt);
  set_csr(mstatus, MSTATUS_IE1 | MSTATUS_FS | MSTATUS_XS | MSTATUS_MPRV);
  clear_csr(mstatus, MSTATUS_VM | MSTATUS_PRV1);
  set_csr(mstatus, (long)VM_SV32 << __builtin_ctzl(MSTATUS_VM));
  //set_csr(mstatus, (long)UA_RV32 << __builtin_ctzl(MSTATUS_UA));

  seed = 1 + (seed % MAX_TEST_PAGES);
  freelist_head = &freelist_nodes[0];
  freelist_tail = &freelist_nodes[MAX_TEST_PAGES-1];
  for (i = 0; i < MAX_TEST_PAGES; i++)
  {
    freelist_nodes[i].addr = (MAX_TEST_PAGES + seed)*PGSIZE;
    freelist_nodes[i].next = &freelist_nodes[i+1];
    seed = LFSR_NEXT(seed);
  }
  freelist_nodes[MAX_TEST_PAGES-1].next = 0;

  trapframe_t tf;
  //memset(&tf, 0, sizeof(tf));
  tf.epc = test_addr;
  //pop_tf(&tf);
}

BOOT_CODE VISIBLE void
init_kernel(
    paddr_t ui_p_reg_start,
    paddr_t ui_p_reg_end,
    int32_t pv_offset,
    vptr_t  v_entry
)
{
  
  printstr("********* Platform Information ********** \n");
  //init_plat();
  
  //vm_boot(&init_kernel, 1337);
  printstr("Initializing platform ...... \n");
  printstr("Entered the kernel \n");
  
  //printf("Entered the kernel \n");
  //putDebugChar('X');
  terminate(0);
  while(1);
}

