#include <string.h>

#include <taskLib.h>

#include "../private.h"

#define MAX_TASKS 1024

/* 
 * We use global variables to avoid having anything from this
 * procedure on the stack as we are clearing it!!!
 * We are task locked so these shouldn't get corrupted 
 */

static int       id_list[MAX_TASKS];
static int       i,num_tasks;
static TASK_DESC info;
static char     *p;
static unsigned  delta;

/*--------------------------------------------------------- */
void
lub_heap_clean_stacks(void)
{
    /* disable task switching */
    taskLock();
    
    num_tasks = taskIdListGet(id_list,MAX_TASKS);
    
    /* deschedule to ensure that the current task stack details are upto date */
    taskDelay(1);
    
    /* iterate round the tasks in the system */
    for(i = 0;
        i < num_tasks;
        i++)
    {
        if(OK == taskInfoGet(id_list[i],&info))
        {
            p     = info.td_pStackBase - info.td_stackHigh;
            delta = info.td_stackHigh - info.td_stackCurrent;
            /* now clean the stack */
            for(;
                delta--;
                p++)
            {
                *p = 0xCC;
            }
        }
    }
    
    /* reenable task switching */
    taskUnlock();
}
/*--------------------------------------------------------- */