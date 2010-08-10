/*
 * This program provides a wrapper to another command but ensuring that the
 * lubheap memory management system is used in preference to the native one
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "lub/heap.h"


void
usage(const char *prog)
{
    printf("%s [-l] command [args ...]\n"
           "\n"
           "    This application invokes the specified\n"
           "    command utilsing the lubheap memory management\n"
           "    library.\n"
           "\n"
           "    -l      - enable leak detection\n"
           "    command - the program to run\n\n", 
           prog);
    exit(-1);
}

int
main(int    argc, 
     char **argv, 
     char **envp)
{
    const char *prog = *argv;
    const char *path;
    --argc,++argv;

    if(!argc)
    {
        usage(prog);
    }
    path = *argv;

    /* check to see whether this program exists in the current path */
    

    /* replace the current process having first set up the memory management system */
    lub_heap_init(prog);    

    return execve(path,argv,envp);
}
