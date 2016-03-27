/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

/* WARNING: constants also defined in plat/machine/hardware.h */
KERNEL_BASE   = 0xFFFFFFFF80000000;
PHYS_BASE     = 0x0000000000a00000;
KERNEL_OFFSET = KERNEL_BASE - PHYS_BASE; 

SECTIONS
{
    . = . + KERNEL_BASE;

    .vector 0xFFFFFFFF80000000:
    {
        *(.vector) 
    }

    . = ALIGN(4M);

    .boot . : AT(ADDR(.boot) - KERNEL_OFFSET)
    {
        *(.boot.text)
        *(.boot.rodata)
        *(.boot.data)
        . = ALIGN(64K);
    }


    ki_boot_end = .;
    
    .text . : AT(ADDR(.text) - KERNEL_OFFSET)
    {
        /* Sit inside a large frame */
        . = ALIGN(64K);
        *(.vectors)

        /* Fastpath code */
        *(.vectors.fastpath_call)
        *(.vectors.fastpath_reply_wait)
        *(.vectors.text)

        /* Anything else that should be in the vectors page. */
        *(.vectors.*)

        /* Hopefully all that fits into 4K! */

        /* Standard kernel */
        *(.text)
    }

    /* Start of data section */
    _sdata = .;
    .sdata : {
        _gp = . + 0x800;
        *(.sdata*)
    }
    .srodata : {
        *(.srodata*)
    }

    .rodata . : AT(ADDR(.rodata) - KERNEL_OFFSET)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data . : AT(ADDR(.data) - KERNEL_OFFSET)
    {
        *(.data)
    }

    .bss . : AT(ADDR(.bss) - KERNEL_OFFSET)
    {
        *(.bss)

        /* 4k breakpoint stack */
        _breakpoint_stack_bottom = .;
        . = . + 4K;
        _breakpoint_stack_top = .;
        
        /* large data such as the globals frame and global PD */
        *(.bss.aligned)
    }

    . = ALIGN(4K);
    ki_end = .;

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }

   /* Make this linker variable to make the assembler happy - used in trap.S and corresponds 
    * to PPTR_KERNEL_STACK_TOP 
    */
   _kernel_stack = 0xFFFFFFFF8FFF1FEA;
}
