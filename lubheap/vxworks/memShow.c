#include <memLib.h>

#include "private.h"

/*-------------------------------------------------------- */
STATUS
memPartInfoGet(PART_ID         partId, 
               MEM_PART_STATS *ppartStats)
{
    partition_t       *this = (partition_t*)partId;
    lub_heap_stats_t stats;
    
    semTake(&this->sem,WAIT_FOREVER);
    lub_heap__get_stats(this->heap,&stats);
    ppartStats->maxBlockSizeFree = lub_heap__get_max_free(this->heap);
    semGive(&this->sem);

    /* now fill out the statistics */
    ppartStats->numBytesFree   = stats.free_bytes;
    ppartStats->numBlocksFree  = stats.free_blocks;
    ppartStats->numBytesAlloc  = stats.alloc_bytes;
    ppartStats->numBlocksAlloc = stats.alloc_blocks;

    return OK;
}
/*-------------------------------------------------------- */
STATUS
memPartShow(PART_ID partId, 
            int     type)
{
    partition_t *this = (partition_t*)partId;
    
    semTake(&this->sem,WAIT_FOREVER);

    lub_heap_show(this->heap,type);

    semGive(&this->sem);

    return OK;
}
/*-------------------------------------------------------- */
void 
memShow(int type)
{
    memPartShow(memSysPartId,type);
}
/*-------------------------------------------------------- */
void
memShowInit (void)
{
}
/*--------------------------------------------------------- */
