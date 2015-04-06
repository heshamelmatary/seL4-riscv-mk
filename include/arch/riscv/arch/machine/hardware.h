/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ARCH_MACHINE_HARDWARE_H
#define __ARCH_MACHINE_HARDWARE_H

#include <config.h>
#include <arch/types.h>
#include <arch/linker.h>

/* Include cache, MMU related functions */

#ifndef __ASSEMBLER__

#include <arch/types.h>

enum vm_fault_type {
    RISCVDataAbort = 0,
    RISCVPrefetchAbort = 1 
};
typedef uint32_t vm_fault_type_t;

enum vm_page_size {
    RISCVSmallPage,
    RISCVLargePage,
    RISCVSection,
    RISCVSuperSection
};
typedef uint32_t vm_page_size_t;

enum frameSizeConstants {
    RISCVSmallPageBits    = 12, 
    RISCVLargePageBits    = 16, 
    RISCVSectionBits      = 20, 
    RISCVSuperSectionBits = 24
};

static inline unsigned int CONST
pageBitsForSize(vm_page_size_t pagesize)
{
    switch (pagesize) {
    case RISCVSmallPage:
        return RISCVSmallPageBits;

    case RISCVLargePage:
        return RISCVLargePageBits;

    case RISCVSection:
        return RISCVSectionBits;

    case RISCVSuperSection:
        return RISCVSuperSectionBits;

    default:
        fail("Invalid page size");
    }
}

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_MACHINE_HARDWARE_H */
