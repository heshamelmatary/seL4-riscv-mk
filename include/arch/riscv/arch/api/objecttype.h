#ifndef __ARCH_OBJECT_TYPE_H
#define __ARCH_OBJECT_TYPE_H

typedef enum _object {
    seL4_RISCV_4K = seL4_NonArchObjectTypeCount,
    seL4_RISCV_PageTableObject,
    seL4_RISCV_PageDirectoryObject,
    seL4_ObjectTypeCount
} seL4_ArchObjectType;
typedef uint32_t object_t;

#endif
