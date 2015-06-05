/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ARCH_OBJECTTYPE_H
#define __ARCH_OBJECTTYPE_H

typedef enum _object {
    seL4_RISCV_4K = seL4_NonArchObjectTypeCount,
    //seL4_RISCV_LargePage,
    seL4_RISCV_PageTableObject,
    seL4_RISCV_PageDirectoryObject,
    seL4_ObjectTypeCount
} seL4_ArchObjectType;

typedef uint32_t object_t;

//#define seL4_RISCV_4M seL4_RISCV_LargePage
#endif /* __ARCH_OBJECTTYPE_H */

