/*
 * memPartLib.c
 *
 * We undefine the public functions
 * (just in case they've been MACRO overriden
 */
#undef free
#undef malloc
#undef memAddToPool
#undef memPartAddToPool
#undef memPartAlignedAlloc
#undef memPartAlloc
#undef memPartBlockIsValid
#undef memPartCreate
#undef memPartFree
#undef memPartInit
#undef memPartLibInit

#include <string.h>
#include <memPartLib.h>

#define  __PROTOTYPE_5_0
#include <semLib.h>
#include <logLib.h>
#include <taskLib.h>

#include "private.h"

/* partition used for the system heap */
static partition_t memSysPartition;

/* this must be assigned at initialisation as clients classes can be
 * initialised before the memory library, and require the value of this
 * to be set (even though they don't use the pool at that point) 
 */
PART_ID memSysPartId = (PART_ID)&memSysPartition;

static int       memPartLibInstalled;

static OBJ_CLASS memPartClass;
CLASS_ID         memPartClassId = &memPartClass;

static OBJ_CLASS memPartInstrClass;
CLASS_ID         memPartInstClassId = &memPartInstrClass;

unsigned memPartOptionsDefault = 
    (MEM_BLOCK_CHECK | MEM_ALLOC_ERROR_LOG_FLAG | MEM_BLOCK_ERROR_LOG_FLAG);

/*-------------------------------------------------------- */
static void
memPartAllocError(partition_t *this,
                  const char  *where,
                  size_t       size)
{
    logMsg("%s: block too big - %d in partition %#x\n",
           where,
           size,
           this);
}
/*-------------------------------------------------------- */
static void
memPartBlockError(partition_t *this,
                  const char  *where,
                  void        *block)
{
    logMsg("%s: invalid block %#x in partition %#x\n",
           where,
           block,
           this);
}
/*-------------------------------------------------------- */
void
lubheap_vxworks_check_status(partition_t      *this,
                             lub_heap_status_t status,
                             const char       *where,
                             void             *block,
                             size_t            size)
{
    switch(status)
    {
        /*------------------------------------------------- */
        case LUB_HEAP_CORRUPTED:
        case LUB_HEAP_INVALID_POINTER:
        case LUB_HEAP_DOUBLE_FREE:
        {
            if(this->options & MEM_BLOCK_CHECK)
            {
                if(this->options & MEM_BLOCK_ERROR_LOG_FLAG)
                {
                    memPartBlockError(this,where,block);
                }
                if(this->options & MEM_BLOCK_ERROR_SUSPEND_FLAG)
                {
                    taskSuspend(taskIdSelf());
                }
            }
            break;
        }
        /*------------------------------------------------- */
        case LUB_HEAP_FAILED:
        {
            if(this->options & MEM_ALLOC_ERROR_LOG_FLAG)
            {
                memPartAllocError(this,where,size);
            }
            if(this->options & MEM_ALLOC_ERROR_SUSPEND_FLAG)
            {
                taskSuspend(taskIdSelf());
            }
            break;
        }
        /*------------------------------------------------- */
        case LUB_HEAP_OK:
        {
            break;
        }
        /*------------------------------------------------- */
    }
}
/*-------------------------------------------------------- */
void
free(void *ptr)
{
    memPartFree(memSysPartId,ptr);
}
/*-------------------------------------------------------- */
void *
malloc(size_t nBytes)
{
    return memPartAlloc(memSysPartId,nBytes);
}
/*-------------------------------------------------------- */
void
memAddToPool(char    *pPool, 
             unsigned poolSize)
{
    memPartAddToPool(memSysPartId,pPool,poolSize);
}
/*-------------------------------------------------------- */
STATUS
memPartAddToPool(PART_ID  partId, 
                   char    *pPool,
                   unsigned poolSize)
{
    partition_t  *this = (partition_t*)partId;
    
    semTake(&this->sem,WAIT_FOREVER);
    lub_heap_add_segment(this->heap,pPool,poolSize);
    semGive(&this->sem);

    return OK;
}
/*-------------------------------------------------------- */
void *
memPartAlignedAlloc(PART_ID  partId, 
                    unsigned nBytes,
                    unsigned alignment)
{
    partition_t      *this   = (partition_t*)partId;
    char             *ptr    = NULL;
    lub_heap_status_t status = LUB_HEAP_OK;
    lub_heap_align_t  align  = LUB_HEAP_ALIGN_NATIVE;

    switch(alignment)
    {
        case 4:         align  = LUB_HEAP_ALIGN_2_POWER_2;  break;
        case 8:         align  = LUB_HEAP_ALIGN_2_POWER_3;  break;
        case 16:        align  = LUB_HEAP_ALIGN_2_POWER_4;  break;
        case 32:        align  = LUB_HEAP_ALIGN_2_POWER_5;  break;
        case 64:        align  = LUB_HEAP_ALIGN_2_POWER_6;  break;
        case 128:       align  = LUB_HEAP_ALIGN_2_POWER_7;  break;
        case 256:       align  = LUB_HEAP_ALIGN_2_POWER_8;  break;
        case 512:       align  = LUB_HEAP_ALIGN_2_POWER_9;  break;
        case 1024:      align  = LUB_HEAP_ALIGN_2_POWER_10; break;
        case 2048:      align  = LUB_HEAP_ALIGN_2_POWER_11; break;
        case 4096:      align  = LUB_HEAP_ALIGN_2_POWER_12; break;
        case 8192:      align  = LUB_HEAP_ALIGN_2_POWER_13; break;
        case 16384:     align  = LUB_HEAP_ALIGN_2_POWER_14; break;
        case 32768:     align  = LUB_HEAP_ALIGN_2_POWER_15; break;
        case 65536:     align  = LUB_HEAP_ALIGN_2_POWER_16; break;
        case 131072:    align  = LUB_HEAP_ALIGN_2_POWER_17; break;
        case 262144:    align  = LUB_HEAP_ALIGN_2_POWER_18; break;
        case 524288:    align  = LUB_HEAP_ALIGN_2_POWER_19; break;
        case 1048576:   align  = LUB_HEAP_ALIGN_2_POWER_20; break;
        case 2097152:   align  = LUB_HEAP_ALIGN_2_POWER_21; break;
        case 4194304:   align  = LUB_HEAP_ALIGN_2_POWER_22; break;
        case 8388608:   align  = LUB_HEAP_ALIGN_2_POWER_23; break;
        case 16777216:  align  = LUB_HEAP_ALIGN_2_POWER_24; break;
        case 33554432:  align  = LUB_HEAP_ALIGN_2_POWER_25; break;
        case 67108864:  align  = LUB_HEAP_ALIGN_2_POWER_26; break;
        case 134217728: align  = LUB_HEAP_ALIGN_2_POWER_27; break;
        default:        status = LUB_HEAP_FAILED;           break;
    }
    if(LUB_HEAP_OK == status)
    {
        semTake(&this->sem,WAIT_FOREVER);
        status = lub_heap_realloc(this->heap,&ptr,nBytes,align);
        semGive(&this->sem);
    }
    lubheap_vxworks_check_status(this,
                                 status,
                                 "memPartAlignedAlloc",
                                 NULL,
                                 nBytes);
    return ptr;
}
/*-------------------------------------------------------- */
void *
memPartAlloc(PART_ID  partId, 
             unsigned nBytes)
{
    partition_t      *this = (partition_t*)partId;
    char             *ptr  = NULL;
    lub_heap_status_t status;
    
    semTake(&this->sem,WAIT_FOREVER);
    status = lub_heap_realloc(this->heap,&ptr,nBytes,LUB_HEAP_ALIGN_NATIVE);
    semGive(&this->sem);

    lubheap_vxworks_check_status(this,status,"memPartAlloc",NULL,nBytes);

    return ptr;
}
/*-------------------------------------------------------- */
BOOL
memPartBlockIsValid(PART_ID    partId, 
                    BLOCK_HDR *pHdr,
                    BOOL       isFree)
{
    partition_t *this = (partition_t*)partId;
    bool_t       result;
    
    semTake(&this->sem,WAIT_FOREVER);
    result = lub_heap_validate_pointer(this->heap,(char*)pHdr);
    semGive(&this->sem);

    return (BOOL_TRUE == result) ? TRUE : FALSE;
}
/*-------------------------------------------------------- */
PART_ID
memPartCreate(char    *pPool, 
              unsigned poolSize)
{
    partition_t *this = objAlloc(memPartClassId);
    
    memPartInit((PART_ID)this,pPool,poolSize);
    
    return (PART_ID)this;
}
/*-------------------------------------------------------- */
static void
memPartDestroy(PART_ID partId)
{
    partition_t *this = (partition_t*)partId;
    
    semTake(&this->sem,WAIT_FOREVER);
    lub_heap_destroy(this->heap);
    this->heap = 0;
    objCoreTerminate(&this->objCore);
    semGive(&this->sem);

    objFree(memPartClassId,(char*)this);
}
/*-------------------------------------------------------- */
STATUS
memPartFree(PART_ID partId, 
            char   *pBlock)
{
    partition_t        *this = (partition_t*)partId;
    lub_heap_status_t status;

    if(OK == OBJ_VERIFY(pBlock,memPartClassId))
    {
        /* we are releasing a memory partition */
        memPartDestroy((PART_ID)pBlock);
        status = LUB_HEAP_OK;
    }
    else
    {
        /* we are simply releasing memory */
        semTake(&this->sem,WAIT_FOREVER);
        status = lub_heap_realloc(this->heap,&pBlock,0,LUB_HEAP_ALIGN_NATIVE);
        semGive(&this->sem);
    }

    lubheap_vxworks_check_status(this,status,"memPartFree",pBlock,0);
    
    return (LUB_HEAP_OK == status) ? OK : ERROR;
}
/*-------------------------------------------------------- */
void
memPartInit(PART_ID  partId,
            char    *pPool,
            unsigned poolSize)
{
    partition_t  *this = (partition_t*)partId;
    
    memset(this,0,sizeof(partition_t));
    
    /* initialise the core object */
    objCoreInit(&this->objCore,memPartClassId);

    /* set the default options */
    this->options = memPartOptionsDefault;

    /* initialise the semaphore for the partition */
    semMInit(&this->sem,
             SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    
    /* create the heap */
    this->heap = lub_heap_create(pPool,poolSize);
}
/*-------------------------------------------------------- */
STATUS
memPartLibInit(char    *pPool, 
               unsigned poolSize)
{
    if(!memPartLibInstalled)
    {
        memPartLibInstalled = 1;

        classInit(memPartClassId,
                  sizeof(partition_t),
                  offsetof(partition_t,objCore),
                  (FUNCPTR)memPartCreate,
                  (FUNCPTR)memPartInit,
                  (FUNCPTR)memPartDestroy);
    
        classInstrument(memPartClassId,memPartInstClassId);

        /* now initialise the system partition instance */
        memPartInit((PART_ID)&memSysPartition,
                    pPool,
                    poolSize);

    }
    return OK;
}
/*-------------------------------------------------------- */
