#ifndef __ARCH_MACHINE_H
#define __ARCH_MACHINE_H

#include <arch/types.h>
#include <arch/object/structures.h>
#include <arch/machine/hardware.h>
#include <arch/machine/page.h>
#include <arch/machine/cpu_registers.h>
#include <arch/model/statedata.h>

#define wordBits 32

word_t PURE getRestartPC(tcb_t *thread);
void setNextPC(tcb_t *thread, word_t v);

/* Cleaning memory before user-level access */
static inline void clearMemory(void* ptr, unsigned int bits)
{
    memzero(ptr, BIT(bits));
    /* no cleaning of caches necessary on IA-32 */
}
#endif
