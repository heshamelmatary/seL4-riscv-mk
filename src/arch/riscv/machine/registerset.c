/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <arch/machine/registerset.h>

const register_t msgRegisters[] = {
    X20, X21, X22, X23
};

const register_t frameRegisters[] = {
    X18, X19, X20, X21, X22, X23, X24
};

const register_t gpRegisters[] = {
    X18, X19, X20, X21, X22, X23, X24
};

const register_t exceptionMessage[] = {
    SP /* TODO: Add other realted registers */
};

const register_t syscallMessage[] = {
    X18, X19, X20, X21, X22, X23, X24, X25, SP 
};
