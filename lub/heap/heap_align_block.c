#include <assert.h>
#include "private.h"
#include "context.h"
#include "node.h"

/*--------------------------------------------------------- */
static void
lub_heap_free_leading_memory(lub_heap_t       *this,
                             lub_heap_block_t *block,
                             size_t            size)
{
    lub_heap_block_t *prev_block = lub_heap_block_getprevious(block);
    if(prev_block && prev_block->free.tag.free)
    {
        /* 
         * add the excess bytes to a preceeding free block
         */
        lub_heap_graft_to_top(this,
                              prev_block,
                              block,
                              (size >> 2),
                              BOOL_FALSE,
                              BOOL_FALSE);
    }
    else
    {
        /*
         * convert the excess bytes into a free block
         */
        lub_heap_init_free_block(this,
                                 block,
                                 size,
                                 block->alloc.tag.segment,
                                 BOOL_FALSE);
    }
    this->stats.alloc_bytes -= size;
}
/*--------------------------------------------------------- */
static void
lub_heap_free_trailing_memory(lub_heap_t       *this,
                              lub_heap_block_t *block,
                              size_t            size)
{
    bool_t            done       = BOOL_FALSE;
    lub_heap_block_t *next_block = lub_heap_block_getnext(block);
    lub_heap_tag_t   *tail       = lub_heap_block__get_tail(block);
    char             *ptr        = (char*)&tail[1] - size;
    if(next_block && next_block->free.tag.free)
    {
        /* 
         * try and add the excess bytes to a following free block
         */
        lub_heap_graft_to_bottom(this,
                                 next_block,
                                 ptr,
                                 (size >> 2),
                                 BOOL_FALSE,
                                 BOOL_FALSE);
        done = BOOL_TRUE;
    }
    else if(size >= sizeof(lub_heap_free_block_t))
    {
        /*
         * convert the excess bytes into a free block
         */
        lub_heap_init_free_block(this,
                                 ptr,
                                 size,
                                 BOOL_FALSE,
                                 tail->segment);
        done = BOOL_TRUE;
    }

    if(done)
    {
        /* correct the word count for the block */
        block->alloc.tag.words -= (size >> 2);
        tail          = lub_heap_block__get_tail(block);
        tail->segment = 0;
        tail->free    = 0;
        tail->words   = block->alloc.tag.words;

        this->stats.alloc_bytes -= size;
    }
}
/*--------------------------------------------------------- */
void
lub_heap_align_block(lub_heap_t      *this,
                     lub_heap_align_t alignment,
                     char           **ptr,
                     size_t          *size)
{
    if(*ptr)
    {
        lub_heap_block_t *block = lub_heap_block_from_ptr(*ptr);
        lub_heap_block_t *new_block;
        lub_heap_tag_t   *new_tail;
        /* find the second occurance of an alignment boundary */
        size_t    tmp           = (alignment-1);
        char     *new_ptr       = (char *)(((size_t)*ptr + tmp + alignment) & ~tmp);
        size_t    leading_bytes = (new_ptr - *ptr);
        size_t    trailing_bytes;
        
        /* check this is a allocated block */
        assert(0 == block->alloc.tag.free);
        /*
         * We MUST have left sufficient space to fit a free block and
         * leak dection node if necessary.
         */
        assert(leading_bytes >= (sizeof(lub_heap_free_block_t) + sizeof(lub_heap_node_t)));

        if((0 < lub_heap_frame_count) && (0 == this->suppress))
        {
            /* 
             * If leak detection is enabled then offset the pointer back by the size of 
             * a lub_heap_node; which will be filled out by lub_heap_post_realloc()
             */
            new_ptr       -= sizeof(lub_heap_node_t);
            leading_bytes -= sizeof(lub_heap_node_t);
        }
        /*
         * Create a new header just before the new pointer
         */
        new_block = lub_heap_block_from_ptr(new_ptr);
        new_block->alloc.tag.free    = 0;
        new_block->alloc.tag.segment = 0;
        new_block->alloc.tag.words   = block->alloc.tag.words - (leading_bytes >> 2);

        /* fix the size in the tail */
        new_tail        = lub_heap_block__get_tail(new_block);
        new_tail->words = new_block->alloc.tag.words;

        lub_heap_free_leading_memory(this,
                                     block,
                                     leading_bytes);

        /* identify any trailing excess memory */
        trailing_bytes  = (new_block->alloc.tag.words << 2) - (*size - (alignment << 1));
        
        if(trailing_bytes)
        {
            lub_heap_free_trailing_memory(this,
                                          new_block,
                                          trailing_bytes);
        }
        *size = (new_block->alloc.tag.words << 2); /* size including the lub_heap_alloc_block_t details */
        *ptr  = new_ptr;
    }
}
