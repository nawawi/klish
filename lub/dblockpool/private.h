#include "lub/dblockpool.h"
#include "lub/blockpool.h"

typedef struct _lub_dblockpool_chunk lub_dblockpool_chunk_t;
struct _lub_dblockpool_chunk
{
    lub_dblockpool_chunk_t *next;
    lub_blockpool_t         pool;
    unsigned                count;
};
