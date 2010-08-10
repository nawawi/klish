/*
 * vxworks_partition.c
 */
#include <stdlib.h>
#include <assert.h>

#include <taskLib.h>
#include <taskHookLib.h>

#include "private.h"

static lub_vxworks_partition_t *first_partition;

/*-------------------------------------------------------- */
static void
lub_vxworks_partition_task_delete_hook(WIND_TCB *pTcb)
{
    lub_vxworks_partition_t **ptr;
    int                       tid = (int)pTcb;
    
    /* iterate the list of partitions */
    taskLock();
    for(ptr = &first_partition;
        *ptr;
         ptr = &(*ptr)->m_next_partition)
    {
        int val = taskVarGet(tid,(int*)&(*ptr)->m_local_heap);
        if(ERROR != val)
        {
            /* destroy this local heap */
            lub_partition_destroy_local_heap(&(*ptr)->m_base,(void*)val);

            /* and remove the task variable */
            taskVarDelete(tid,(int*)&(*ptr)->m_local_heap);
        }
    }
    taskUnlock();
}
/*-------------------------------------------------------- */
lub_partition_t *
lub_partition_create(const lub_partition_spec_t *spec)
{
    lub_vxworks_partition_t *this;
    this = calloc(sizeof(lub_vxworks_partition_t),1);
    if(this)
    {
        /* initialise the global mutex */
        if(ERROR == semMInit(&this->m_sem,SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE))
        {
            assert(NULL == "semMInit() failed!");
        }
        
        lub_partition_lock(&this->m_base);
        /* initialise the local task variable system */
        taskVarInit();
        
        /* initialise the base class */
        lub_partition_init(&this->m_base,spec);
        
        /* add to the list of partitions */
        this->m_next_partition = first_partition;
        first_partition        = this;

        if(this == first_partition)
        {
            /* register a task deletion hook */
            taskDeleteHookAdd((FUNCPTR)lub_vxworks_partition_task_delete_hook);
        }
        lub_partition_unlock(&this->m_base);
    }
    return this ? &this->m_base : 0;
}
/*-------------------------------------------------------- */
void
lub_partition_destroy(lub_partition_t *instance)
{
    lub_vxworks_partition_t  *this = (void*)instance;
    lub_vxworks_partition_t **ptr;

    /* finalise the base class */
    lub_partition_fini(&this->m_base);
    
    lub_partition_lock(&this->m_base);
    /* remove from the list of partitions */
    for(ptr = &first_partition;
        *ptr;
        ptr = &(*ptr)->m_next_partition)
    {
        if(&(*ptr)->m_base == instance)
        {
            /* remove from list */
            *ptr = (*ptr)->m_next_partition;
            break;
        }
    }
    if(!first_partition)
    {
        /* deregister delete hook */
        taskDeleteHookDelete((FUNCPTR)lub_vxworks_partition_task_delete_hook);
    }
    lub_partition_unlock(&this->m_base);
    free(this);
}
/*-------------------------------------------------------- */
lub_heap_t *
lub_partition__get_local_heap(lub_partition_t *instance) 
{
    lub_vxworks_partition_t *this = (void*)instance;

    return this->m_local_heap;
}
/*-------------------------------------------------------- */
void
lub_partition__set_local_heap(lub_partition_t *instance,
                              lub_heap_t      *heap) 
{
    lub_vxworks_partition_t *this = (void*)instance;

    if(this->m_local_heap)
    {
        assert(NULL == "Local heap already exists!");
    }
    /* add this memory to the local task */
    taskVarAdd(0,(int*)&this->m_local_heap);

    /* add set the value */
    this->m_local_heap = heap;
}
/*-------------------------------------------------------- */
void
lub_partition_lock(lub_partition_t *instance) 
{
    lub_vxworks_partition_t *this = (void*)instance;
    
    if(ERROR == semTake(&this->m_sem,WAIT_FOREVER))
    {
        assert(NULL == "semTake() failed!");
    }
}
/*-------------------------------------------------------- */
void
lub_partition_unlock(lub_partition_t *instance) 
{
    lub_vxworks_partition_t *this = (void*)instance;
    
    if(ERROR == semGive(&this->m_sem))
    {
        assert(NULL == "semGive() failed!");
    }
}
/*-------------------------------------------------------- */
