/* 
 * heap_show.c
 */
#include <stdio.h>
#include "cache.h"

/*--------------------------------------------------------- */
static void
process_free_block(void     *block,
                   unsigned  index,
                   size_t    size,
                   void     *arg)
{
    /* dump the details for this block */
    printf(" %8p[%"SIZE_FMT"]",block,size);
}
/*--------------------------------------------------------- */
static void
process_segment(void     *segment,
                unsigned  index,
                size_t    size,
                void     *arg)
{
    /* dump the details for this segment */
    printf(" %8p[%"SIZE_FMT"]",segment,size);
}
/*--------------------------------------------------------- */
void
lub_heap_show(lub_heap_t *this,
              bool_t      verbose)
{
    static char     *state_names[] = {"DISABLED","ENABLED"};
    lub_heap_stats_t stats;

    if(BOOL_FALSE == lub_heap_check_memory(this))
    {
        printf("*** HEAP CORRUPTED!!! ***\n");
    }
    if(BOOL_TRUE == verbose)
    {
        printf("HEAP:\n"
               "  %8p\n",(void*)this);
        printf("\nHEAP SEGMENTS:\n");
        lub_heap_foreach_segment(this,process_segment,NULL);
        printf("\n");
        if(this->cache)
        {
            printf("\nCACHE:\n");
            lub_heap_cache_show(this->cache);
        }
        /* dump each free block's details */
        printf("\nFREE BLOCKS:\n");
        lub_heap_foreach_free_block(this,process_free_block,NULL);
        printf("\n");
        printf("\nSUMMARY:\n");
    }
    {
        /*lint -esym(644,cache_stats) may not have been initialized */
        lub_heap_stats_t cache_stats;
        /* get the stats */
        lub_heap__get_stats(this,&stats);
    
        /* dump the statistics for this heap */
        printf("  status      bytes    blocks  avg block  max block   overhead\n");
        printf("  ------ ---------- --------- ---------- ---------- ----------\n");
        
        printf("current\n");
        printf("    free %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT"\n",
               stats.free_bytes,
               stats.free_blocks,
               stats.free_blocks ? stats.free_bytes /stats.free_blocks : 0,
               lub_heap__get_max_free(this),
               stats.free_overhead + stats.segs_overhead);
        printf("   alloc %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10"SIZE_FMT"\n",
               stats.alloc_bytes,
               stats.alloc_blocks,
               stats.alloc_blocks ? stats.alloc_bytes / stats.alloc_blocks : 0,
               "-",
               stats.alloc_overhead);
        printf("  static %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10"SIZE_FMT"\n",
               stats.static_bytes,
               stats.static_blocks,
               stats.static_blocks ? stats.static_bytes / stats.static_blocks : 0,
               "-",
               stats.static_overhead);
        if(this->cache)
        {
            printf(" (cache)\n");
            lub_heap_cache__get_stats(this->cache,&cache_stats);
            printf("    free %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT" %10"SIZE_FMT"\n",
                   cache_stats.free_bytes,
                   cache_stats.free_blocks,
                   cache_stats.free_blocks ? cache_stats.free_bytes /cache_stats.free_blocks : 0,
                   lub_heap_cache__get_max_free(this->cache),
                   cache_stats.free_overhead);
            printf("   alloc %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10"SIZE_FMT"\n",
                   cache_stats.alloc_bytes,
                   cache_stats.alloc_blocks,
                   cache_stats.alloc_blocks ? cache_stats.alloc_bytes / cache_stats.alloc_blocks : 0,
                   "-",
                   cache_stats.alloc_overhead);
        }

        printf("cumulative\n");
        printf("   alloc %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10s\n",
               stats.alloc_total_bytes,
               stats.alloc_total_blocks,
               stats.alloc_total_blocks ? stats.alloc_total_bytes / stats.alloc_total_blocks : 0,
               "-",
               "-");
        if(this->cache)
        {
            printf(" (cache)\n");
            printf("   alloc %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10s\n",
                   cache_stats.alloc_total_bytes,
                   cache_stats.alloc_total_blocks,
                   cache_stats.alloc_total_blocks ? cache_stats.alloc_total_bytes / cache_stats.alloc_total_blocks : 0,
                   "-",
                   "-");
        }
        printf("high-tide\n");
        printf("    free %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10"SIZE_FMT"\n",
               stats.free_hightide_bytes,
               stats.free_hightide_blocks,
               stats.free_hightide_blocks ? stats.free_hightide_bytes /stats.free_hightide_blocks : 0,
               "-",
               stats.free_hightide_overhead);
        printf("   alloc %10"SIZE_FMT" %9"SIZE_FMT" %10"SIZE_FMT" %10s %10"SIZE_FMT"\n",
               stats.alloc_hightide_bytes,
               stats.alloc_hightide_blocks,
               stats.alloc_hightide_blocks ? stats.alloc_hightide_bytes / stats.alloc_hightide_blocks : 0,
               "-",
               stats.alloc_hightide_overhead);
    }
    if(BOOL_TRUE == verbose)
    {
        printf("\nSYSTEM:\n"
               "  tainting(%s), full checking(%s), leak detection(%s),\n"
               "  native alignment of %d bytes\n", 
               state_names[lub_heap_is_tainting()],
               state_names[lub_heap_is_checking()],
               state_names[lub_heap__get_framecount() ? 0 : 1],
               LUB_HEAP_ALIGN_NATIVE);
    }
    printf("\n");
}
