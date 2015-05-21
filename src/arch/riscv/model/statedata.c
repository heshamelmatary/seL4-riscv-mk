/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <util.h>
#include <api/types.h>
#include <arch/types.h>
#include <arch/model/statedata.h>
#include <arch/object/structures.h>
#include <arch/linker.h>
#include <plat/machine/hardware.h>

/* The global frame, mapped in all address spaces */
word_t riscvKSGlobalsFrame[BIT(PAGE_BITS) / sizeof(word_t)] ALIGN_BSS(BIT(PAGE_BITS));

/* The top level asid mapping table */
//asid_pool_t *riscvKSASIDTable[BIT(asidHighBits)];

/* The hardware ASID to virtual ASID mapping table */
//asid_t riscvKSHWASIDTable[BIT(hwASIDBits)];
//hw_asid_triscvKSNextASID;

/* This is only needed for 64-bit implementation, keep it for future */
uint32_t l3pt[PTES_PER_PT] __attribute__((aligned(4096))) VISIBLE;

