/*
 * blockpool__get_stats->c
 */
#include "private.h"

/*--------------------------------------------------------- */
void
lub_blockpool__get_stats(lub_blockpool_t *this,
                         lub_blockpool_stats_t *stats)
{
    stats->block_size            = this->m_block_size;
    stats->num_blocks            = this->m_num_blocks;
    stats->alloc_blocks          = this->m_alloc_blocks;
    stats->alloc_bytes           = stats->alloc_blocks * stats->block_size;
    stats->free_blocks           = stats->num_blocks - stats->alloc_blocks;
    stats->free_bytes            = stats->free_blocks * stats->block_size;
    stats->alloc_total_blocks    = this->m_alloc_total_blocks;
    stats->alloc_total_bytes     = stats->alloc_total_blocks * stats->block_size;
    stats->alloc_hightide_blocks = this->m_alloc_hightide_blocks;
    stats->alloc_hightide_bytes  = stats->alloc_hightide_blocks * stats->block_size;
    stats->free_hightide_blocks  = stats->num_blocks - stats->alloc_hightide_blocks;
    stats->free_hightide_bytes   = stats->free_hightide_blocks * stats->block_size;
    stats->alloc_failures        = this->m_alloc_failures;
}
/*--------------------------------------------------------- */
