#include <semLib.h>
#include <private/semLibP.h>

#include "../private.h"
#include "../context.h"

static SEMAPHORE leak_sem;

/*--------------------------------------------------------- */
static void
meta_init(void)
{
    static bool_t initialised = BOOL_FALSE;
    if(BOOL_FALSE == initialised)
    {
        initialised = BOOL_TRUE;
        /* initialise the semaphore for the partition */
        semMInit(&leak_sem,SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_leak_mutex_lock(void)
{
    meta_init();
    semTake(&leak_sem,WAIT_FOREVER);
}
/*--------------------------------------------------------- */
void
lub_heap_leak_mutex_unlock(void)
{
    semGive(&leak_sem);
}
/*--------------------------------------------------------- */
