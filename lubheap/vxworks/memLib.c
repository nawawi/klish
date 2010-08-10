/*
 * memLib.c
 *
 * We undefine the public functions
 * (just in case they've been MACRO overriden
 */
#undef calloc
#undef cfree
#undef memFindMax
#undef memInit
#undef memLibInit
#undef memOptionsSet
#undef memPartFindMax
#undef memPartOptionsSet
#undef memPartRealloc
#undef memalign
#undef realloc
#undef valloc

#include <string.h>

#include <stdlib.h>
#include <memPartLib.h>
#include <memLib.h>
#include <private/funcBindP.h>

#include "private.h"

#define VX_PAGE_SIZE 4096

/*-------------------------------------------------------- */
void *
calloc(size_t elemNum,
       size_t elemSize)
{
    size_t nBytes = elemNum * elemSize;
    char  *ptr    = malloc(nBytes);

    memset(ptr,0,nBytes);
    
    return ptr;
}
/*-------------------------------------------------------- */
STATUS
cfree(char *pBlock)
{
    return memPartFree(memSysPartId,pBlock);
}
/*-------------------------------------------------------- */
int
memFindMax(void)
{
    return memPartFindMax(memSysPartId);
}
/*-------------------------------------------------------- */
STATUS
memLibInit(char    *pPool, 
           unsigned poolSize)
{
    memPartLibInit(pPool,poolSize);
    return OK;
}
/*-------------------------------------------------------- */
STATUS
memInit(char    *pPool, 
        unsigned poolSize)
{
    /* set up the system pointers */
    _func_valloc   = (FUNCPTR) valloc;
    _func_memalign = (FUNCPTR) memalign;

    return memLibInit(pPool,poolSize);
}
/*-------------------------------------------------------- */
void
memOptionsSet (unsigned options)
{
    memPartOptionsSet(memSysPartId,options);
}
/*-------------------------------------------------------- */
int 
memPartFindMax(PART_ID partId)
{
    partition_t *this = (partition_t *)partId;
    size_t     result;
    
    semTake(&this->sem,WAIT_FOREVER);
    result = lub_heap__get_max_free(this->heap);
    semGive(&this->sem);
    
    return result;
}
/*-------------------------------------------------------- */
STATUS
memPartOptionsSet(PART_ID  partId, 
                  unsigned options)
{
    partition_t  *this = (partition_t*)partId;
    
    this->options = options;
 
    return OK;
}
/*-------------------------------------------------------- */
void *   
memPartRealloc(PART_ID partId, 
               char   *pBlock, 
               unsigned nBytes)
{
    partition_t        *this = (partition_t *)partId;
    lub_heap_status_t status;

    semTake(&this->sem,WAIT_FOREVER);
    status = lub_heap_realloc(this->heap,&pBlock,nBytes,LUB_HEAP_ALIGN_NATIVE);
    semGive(&this->sem);

    lubheap_vxworks_check_status(this,status,"memPartRealloc",pBlock,nBytes);

    return (LUB_HEAP_OK == status) ? pBlock : NULL;
}
/*-------------------------------------------------------- */
void *
memalign(unsigned alignment, 
         unsigned size)
{
    return memPartAlignedAlloc(memSysPartId,size,alignment);
}
/*-------------------------------------------------------- */
void *
realloc(void *ptr, size_t nBytes)
{
    return memPartRealloc(memSysPartId,ptr,nBytes);
}
/*-------------------------------------------------------- */
void *
valloc(unsigned size)
{
    return memalign(VX_PAGE_SIZE,size);
}
/*-------------------------------------------------------- */
