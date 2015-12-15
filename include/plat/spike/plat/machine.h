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

#ifndef __PLAT_MACHINE_H
#define __PLAT_MACHINE_H

#define KERNEL_TIMER_IRQ INTERRUPT_TIMER

#define N_INTERRUPTS 2

enum IRQConstants {
    INTERRUPT_SW = 0,
    INTERRUPT_TIMER = 1,
    /* We don't need other IRQs for now at least */
    maxIRQ = 3 
} platform_interrupt_t;

enum irqNumbers {
    irqInvalid = 3
};

typedef uint32_t interrupt_t;
typedef uint32_t irq_t;

#endif
