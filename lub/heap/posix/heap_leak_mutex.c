#include <pthread.h>
#include <stdio.h>

#include "../private.h"
#include "../context.h"

static pthread_mutex_t leak_mutex = PTHREAD_MUTEX_INITIALIZER;
static lub_heap_leak_t instance;
/*--------------------------------------------------------- */
void
lub_heap_leak_mutex_lock(void)
{
    int status = pthread_mutex_lock(&leak_mutex);
    if(0 != status)
    {
        perror("pthread_mutex_lock() failed");
    }
}
/*--------------------------------------------------------- */
void
lub_heap_leak_mutex_unlock(void)
{
    int status = pthread_mutex_unlock(&leak_mutex);
    if(0 != status)
    {
        perror("pthread_mutex_unlock() failed");
    }
}
/*--------------------------------------------------------- */
lub_heap_leak_t *
lub_heap_leak_instance(void)
{
    lub_heap_leak_mutex_lock();
    return &instance;
}
/*--------------------------------------------------------- */
void
lub_heap_leak_release(lub_heap_leak_t *instance)
{
    lub_heap_leak_mutex_unlock();
}
/*--------------------------------------------------------- */
bool_t
lub_heap_leak_query_node_tree(void)
{
    return BOOL_TRUE;
}
/*--------------------------------------------------------- */
bool_t
lub_heap_leak_query_clear_node_tree(void)
{
    return BOOL_TRUE;
}
/*--------------------------------------------------------- */

