#
# Copyright 2014, General Dynamics C4 Systems
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(GD_GPL)
#

DIRECTORIES += src/arch/riscv/machine

ARCH_C_SOURCES += machine/registerset.c
#                  machine/cache.c \
#                  machine/errata.c

#ARCH_C_SOURCES += machine/priv_timer.c


ifdef DEBUG
    ARCH_C_SOURCES += machine/debug.c machine/capdl.c
endif
