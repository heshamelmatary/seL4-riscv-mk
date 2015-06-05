/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <types.h>
#include <api/failures.h>
#include <kernel/vspace.h>
#include <object/structures.h>
#include <arch/machine.h>
#include <arch/model/statedata.h>
#include <arch/object/objecttype.h>

deriveCap_ret_t
Arch_deriveCap(cte_t *slot, cap_t cap)
{
    deriveCap_ret_t ret;

    switch (cap_get_capType(cap)) {
    case cap_page_table_cap:
        ret.cap = cap_page_table_cap_set_capPTMappedObject(cap, 0);
        ret.status = EXCEPTION_NONE;
        return ret;

    case cap_page_directory_cap:
        ret.cap = cap_page_directory_cap_set_capPDBasePtr(cap, 0); 
        ret.status = EXCEPTION_NONE;
        return ret;

    case cap_frame_cap:
        ret.cap = cap_frame_cap_set_capFMappedObject(cap, 0);
        ret.status = EXCEPTION_NONE;
        return ret;

    default:
        /* This assert has no equivalent in haskell,
         * as the options are restricted by type */
        fail("Invalid arch cap type");
    }
}

cap_t CONST
Arch_updateCapData(bool_t preserve, word_t data, cap_t cap)
{
    return cap;
}

cap_t CONST
Arch_maskCapRights(cap_rights_t cap_rights_mask, cap_t cap)
{
        return cap;
}

cap_t
Arch_finaliseCap(cap_t cap, bool_t final)
{
    return cap_null_cap_new();
}

static cap_t CONST
resetMemMapping(cap_t cap)
{
    return cap;
}

cap_t
Arch_recycleCap(bool_t is_final, cap_t cap)
{
    return cap_null_cap_new();
}

bool_t CONST
Arch_hasRecycleRights(cap_t cap)
{
        return true;
}


bool_t CONST
Arch_sameRegionAs(cap_t cap_a, cap_t cap_b)
{
    return false;
}


bool_t CONST
Arch_sameObjectAs(cap_t cap_a, cap_t cap_b)
{
  return 0;
}

word_t
Arch_getObjectSize(word_t t)
{
        return 0;
}

cap_t Arch_createObject(object_t t, void *regionBase, int userSize, bool_t
deviceMemory)
{
}

exception_t
Arch_decodeInvocation(word_t label, unsigned int length, cptr_t cptr,
                      cte_t *slot, cap_t cap, extra_caps_t extraCaps,
                      word_t *buffer)
{
    switch (cap_get_capType(cap)) {
    case cap_page_directory_cap:
    case cap_page_table_cap:
    cap_frame_cap:
    		return decodeRISCVMMUInvocation(label, length, cptr, slot, cap, extraCaps, buffer);
    default: printf("Not page_cap");
    }
}

void
Arch_prepareThreadDelete(tcb_t *thread)
{
    /* No action required on RISCV. */
}

bool_t
Arch_isFrameType(word_t t)
{
    switch (t) {
    case seL4_RISCV_Page:
        return true;
    default:
        return false;
    }
}
