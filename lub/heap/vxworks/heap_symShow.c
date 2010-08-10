#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <symLib.h>
#include <sysSymTbl.h>
#include <taskLib.h>

#include "../private.h"
#include "../context.h"

/*--------------------------------------------------------- */
/* VxWorks version of this doesn't use the program_name */
void
lub_heap_init(const char *program_name)
{
    program_name = program_name;
    
    /* switch on leak detection */
    lub_heap__set_framecount(MAX_BACKTRACE);
}
/*--------------------------------------------------------- */
void
lub_heap_symShow(unsigned address)
{
    int        value;
    char     * name;
    SYM_TYPE   type;
    
    if(OK == symByValueFind(sysSymTbl,address,&name,&value,&type))
    {
        printf(" %s+%d",name,address-value);
        free(name);
    }
    else
    {
        printf("UNKNOWN @ %p",(void*)address);
    }
}
/*--------------------------------------------------------- */
bool_t
lub_heap_symMatch(unsigned    address,
                  const char *substring)
{
    bool_t result = BOOL_FALSE;
    int        value;
    char     * name;
    SYM_TYPE   type;
    
    if(OK == symByValueFind(sysSymTbl,address,&name,&value,&type))
    {
        if(strstr(name,substring))
        {
            result = TRUE;
        }
        free(name);
    }
    return result;
}
/*--------------------------------------------------------- */
int
leakEnable(unsigned frame_count)
{
    taskLock();
    
    lub_heap__set_framecount(frame_count ? frame_count : MAX_BACKTRACE);

    taskUnlock();
    
    return 0;
}
/*--------------------------------------------------------- */
int
leakDisable(void)
{
    taskLock();

    lub_heap__set_framecount(0);

    taskUnlock();
    
    return 0;
}
/*--------------------------------------------------------- */
int
leakShow(unsigned    how,
         const char *substring)
{
    /* let's go low priority for this... */
    int old_priority = taskPriorityGet(taskIdSelf(),&old_priority);

    taskPrioritySet(taskIdSelf(),254);
    lub_heap_leak_report(how,substring);
    taskPrioritySet(taskIdSelf(),old_priority);

    return 0;
}
/*--------------------------------------------------------- */
int
leakScan(unsigned    how,
         const char *substring)
{
    /* let's go low priority for this... */
    int old_priority = taskPriorityGet(taskIdSelf(),&old_priority);

    taskPrioritySet(taskIdSelf(),254);
    lub_heap_leak_scan();
    lub_heap_leak_report(how,substring);
    taskPrioritySet(taskIdSelf(),old_priority);
    
    return 0;
}
/*--------------------------------------------------------- */
int
leakHelp(void)
{
    printf("The following commands can be used for leak detection:\n\n");
    printf("  leakHelp                    - Display this command summary.\n\n");
    printf("  leakEnable [frame_count]    - Enable leak detection collecting the \n"
           "                                specified number of stack frames.\n\n");
    printf("  leakDisable                 - Disable leak detection, clearing out \n"
           "                                any currently monitored allocations\n\n");
    printf("  leakScan [how] [,substring] - Scan and show memory leak details.\n"
           "                                'how' can be 0 - leaks only [default]\n"
           "                                             1 - leaks and partials\n"
           "                                             2 - all monitored allocations\n\n"
           "                                'substring' can be specified to only display contexts\n"
           "                                            which contain the string in their backtrace\n\n");
    printf("  leakShow [how] [,substring] - Display details for currently monitored allocations.\n"
           "                                Leak information will be as per the last scan performed.\n\n");
    printf("A 'leak' occurs when there is no reference to any of the memory\n"
           "within an allocated block.\n\n"
           "A 'partial' leak occurs when there is no reference to the start of\n"
           "an allocated block of memory.\n\n"); 
    return 0;
}
/*--------------------------------------------------------- */
