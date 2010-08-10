/*
 * partition_init.c
 */
#include <stdlib.h>
#include <assert.h>

#include "private.h"
/*-------------------------------------------------------- */
void
lub_partition_kill(lub_partition_t *this)
{
    this->m_dying = BOOL_TRUE;
    /* try and die immediately... */
    lub_partition_time_to_die(this);
}
/*-------------------------------------------------------- */
void
lub_partition_init(lub_partition_t            *this,
                   const lub_partition_spec_t *spec)
{
    this->m_spec              = *spec;
    if(!this->m_spec.sysalloc)
    {
        this->m_spec.sysalloc = malloc;
    }
    this->m_partition_ceiling = spec->memory_limit ? (spec->memory_limit - sizeof(lub_partition_t)) : -1;
    this->m_dying             = BOOL_FALSE;
    this->m_global_heap       = 0; /* do this on demand */
}
/*-------------------------------------------------------- */
