/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <machine/assembler.h>

.global idle_thread

.section .text, "ax"

BEGIN_FUNC(idle_thread)
1:
    j 1b
END_FUNC(idle_thread)

.bss
.balign  4096
_idle_tls:
.space  8192

