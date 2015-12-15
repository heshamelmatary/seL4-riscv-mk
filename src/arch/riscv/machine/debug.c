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

#include <stdint.h>
#include <util.h>
//#include <arch/machine/debug.h>
#include <machine/io.h>
#include <model/statedata.h>

typedef uint32_t vector_t;
typedef void (*break_handler_t)(user_context_t *context);
typedef void (*catch_handler_t)(user_context_t *context, vector_t vector);

void
debug_init(void)
{
}

void
software_breakpoint(uint32_t va, user_context_t *context)
{
}

void
breakpoint_multiplexer(uint32_t va, user_context_t *context)
{
}

int
set_breakpoint(uint32_t va, break_handler_t handler)
{
  return 0;
}

void
clear_breakpoint(uint32_t va)
{
}

catch_handler_t catch_handler VISIBLE;

void
set_catch_handler(catch_handler_t handler)
{
}

void
catch_vector(vector_t vector)
{
}

void
uncatch_vector(vector_t vector)
{
}
