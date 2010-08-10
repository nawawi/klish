#include "private.h"

/*--------------------------------------------------------- */
void
lub_dblockpool_init(lub_dblockpool_t *this,
                    size_t            block_size,
                    unsigned          chunk_size,
                    unsigned          max_chunks)
{
    this->first_chunk = NULL;
    this->block_size  = block_size;
    this->chunk_size  = chunk_size;
    this->max_chunks  = max_chunks;
}
/*--------------------------------------------------------- */
