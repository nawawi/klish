/*
 * posix_partition.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "private.h"

extern void sysheap_suppress_leak_detection(void);
extern void sysheap_restore_leak_detection(void);


typedef struct _key_data key_data_t;
struct _key_data
{
    lub_partition_t *partition;
    lub_heap_t      *local_heap;
};
/*-------------------------------------------------------- */
static void
lub_posix_partition_destroy_key(void *arg)
{
    key_data_t *key_data = arg;
    lub_partition_destroy_local_heap(key_data->partition,key_data->local_heap);
    lub_partition_global_realloc(key_data->partition,(char**)key_data,0,LUB_HEAP_ALIGN_NATIVE);
}
/*-------------------------------------------------------- */
lub_partition_t *
lub_partition_create(const lub_partition_spec_t *spec)
{
    lub_posix_partition_t *this;
    lub_partition_sysalloc_fn *alloc_fn = spec->sysalloc;
    if(!alloc_fn)
    {
        alloc_fn = malloc;
    }
    this = alloc_fn(sizeof(lub_posix_partition_t));
    if(this)
    {
        lub_posix_partition_init(this,spec);
    }
    return &this->m_base;
}
/*-------------------------------------------------------- */
void
lub_posix_partition_init(lub_posix_partition_t      *this,
                         const lub_partition_spec_t *spec)
{
    memset(this,sizeof(*this),0);
    /* initialise the base class */
    lub_partition_init(&this->m_base,spec);
    
    /* initialise the global mutex */
    if(0 != pthread_mutex_init(&this->m_mutex,0))
    {
        perror("pthread_mutex_init() failed!");
    }
    /* initialise the local key */
    if(0 != pthread_key_create(&this->m_key,
                               lub_posix_partition_destroy_key))
    {
        perror("pthread_key_create() failed!");
    }
}
/*-------------------------------------------------------- */
void
lub_partition_destroy(lub_partition_t *instance)
{
    lub_posix_partition_t *this = (void*)instance;

    /* finalise the base class */
    lub_partition_fini(&this->m_base);
    
    /* now clean up the global mutex */
    if(0 != pthread_mutex_destroy(&this->m_mutex))
    {
        perror("pthread_mutex_destroy() failed!");
    }

    /* now clean up the local key */
    if(0 != pthread_key_delete(this->m_key))
    {
        perror("pthread_key_delete() failed!");
    }
    
    free(this);
}
/*-------------------------------------------------------- */
lub_heap_t *
lub_partition__get_local_heap(lub_partition_t *instance) 
{
    lub_heap_t            *result   = 0;
    lub_posix_partition_t *this     = (void*)instance;
    key_data_t            *key_data = pthread_getspecific(this->m_key);
    if(key_data)
    {
        result = key_data->local_heap;
    }
    return result;
}
/*-------------------------------------------------------- */
void
lub_partition__set_local_heap(lub_partition_t *instance,
                              lub_heap_t      *heap) 
{
    lub_posix_partition_t *this     = (void*)instance;
    key_data_t            *key_data = 0;

    #if defined(__CYGWIN__)
    /* cygwin seems to leak key_data!!! so we ignore this for the time being... */
    lub_heap_leak_suppress_detection(instance->m_global_heap);
    #endif /* __CYGWIN__*/
    lub_partition_global_realloc(instance,(char**)&key_data,sizeof(key_data_t),LUB_HEAP_ALIGN_NATIVE);
    #if defined(__CYGWIN__)
    /* cygwin seems to leak key_data!!! so we ignore this for the time being... */
    lub_heap_leak_restore_detection(instance->m_global_heap);
    #endif /* __CYGWIN__*/
    assert(key_data);
    key_data->partition  = instance;
    key_data->local_heap = heap;
    if(0 != pthread_getspecific(this->m_key))
    {
        assert(NULL == "Local heap already exists!");
    }
    if(0 != pthread_setspecific(this->m_key,key_data))
    {
        perror("pthread_setspecific() failed!");
    }
}
/*-------------------------------------------------------- */
void
lub_partition_lock(lub_partition_t *instance) 
{
    lub_posix_partition_t *this = (void*)instance;
    
    if(0 != pthread_mutex_lock(&this->m_mutex))
    {
        perror("pthread_mutex_lock() failed!");
    }
}
/*-------------------------------------------------------- */
void
lub_partition_unlock(lub_partition_t *instance) 
{
    lub_posix_partition_t *this = (void*)instance;
    
    if(0 != pthread_mutex_unlock(&this->m_mutex))
    {
        perror("pthread_mutex_unlock() failed!");
    }
}
/*-------------------------------------------------------- */
