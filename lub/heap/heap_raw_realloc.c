#include <string.h>
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
lub_heap_status_t
lub_heap_raw_realloc(lub_heap_t      *this,
                     char           **ptr,
                     size_t           requested_size,
                     lub_heap_align_t alignment)
{
    lub_heap_status_t result               = LUB_HEAP_FAILED;
    lub_heap_block_t *block                = NULL;
    words_t           words;
    char             *new_ptr              = NULL;
    const size_t      MIN_SIZE             = sizeof(lub_heap_free_block_t);
    /* make the minimum alignment leave enough space for 
     * (sizeof(lub_heap_free_block_t) + sizeof(lub_heap_alloc_block_t) + sizeof(lub_heap_node_t))
     * approx 72 bytes on 64-bit architecture
     */
    const lub_heap_align_t MIN_ALIGN_NON_NATIVE = LUB_HEAP_ALIGN_2_POWER_7;
    size_t                 size                 = requested_size;
    
    do
    {
        if(NULL == ptr) break; /* The client MUST give us a pointer to play with */
        
        if(LUB_HEAP_ZERO_ALLOC == *ptr)
        {
            *ptr = NULL;
        }
        if(*ptr && (BOOL_FALSE == lub_heap_validate_pointer(this,*ptr)))
        {
            /* This is not a valid pointer */
            result = LUB_HEAP_INVALID_POINTER;
            break;
        }
        /* check the heap integrity */
        if(BOOL_FALSE == lub_heap_check_memory(this))
        {
            result = LUB_HEAP_CORRUPTED;
            break;
        }
        
        if(size)
        {
            /* make sure we get enough space for the header/tail */
            size += sizeof(lub_heap_alloc_block_t);

            /* make sure we are at least natively aligned */
            size += (LUB_HEAP_ALIGN_NATIVE-1);
            size &= ~(LUB_HEAP_ALIGN_NATIVE-1);

            if(size < MIN_SIZE)
            {
                /* 
                 * we must ensure that any allocated block is at least
                 * large enough to hold a free block node when it is released
                 */
                size = MIN_SIZE;
            }
            if(LUB_HEAP_ALIGN_NATIVE != alignment)
            {
                if(alignment < MIN_ALIGN_NON_NATIVE)
                {
                    /*
                     * ensure that we always leave enough space
                     * to be able to collapse the block to the 
                     * right size
                     */
                    alignment = MIN_ALIGN_NON_NATIVE;
                }
                /* add twice the alignment */
                size  += (alignment << 1);
            }
        }
        words = (size >> 2);

        if(requested_size > size)
        {
            /* the size has wrapped when accounting for overheads */
            break;
        }
        if(NULL != *ptr)
        {
            /* get reference to the current block */
            block = lub_heap_block_from_ptr(*ptr);
    
            /* first of all check this is an allocated block */
            if(1 == block->alloc.tag.free)
            {
                result = LUB_HEAP_DOUBLE_FREE;
                break;
            }
            /* first of all check this is an allocated block */
            if(BOOL_FALSE == lub_heap_block_check(block))
            {
                result = LUB_HEAP_CORRUPTED;
                break;
            }
            /* is the current block large enough for the request */
            if(words && (block->alloc.tag.words >= words))
            {
                lub_heap_block_t *next_block = lub_heap_block_getnext(block);
                words_t           delta      = (block->alloc.tag.words - words);
                lub_heap_tag_t   *tail;
    
                result = LUB_HEAP_OK;
                new_ptr = *ptr;
                
                if(delta && (NULL != next_block) )
                {
                    /* can we graft this spare memory to a following free block? */
                    if(1 == next_block->free.tag.free)
                    {
                        
                        block->alloc.tag.words = words;
                        tail = lub_heap_block__get_tail(block);
                        tail->words   = words;
                        tail->free    = 0;
                        tail->segment = 0;
                        
                        lub_heap_graft_to_bottom(this,
                                                 next_block,
                                                 &tail[1],
                                                 delta,
                                                 BOOL_FALSE,
                                                 BOOL_FALSE);
                        this->stats.alloc_bytes -= (delta << 2);
                        break;
                    }
                }
                /* Is there enough space to turn the spare memory into a free block? */
                if(delta >= (sizeof(lub_heap_free_block_t) >> 2))
                {
                    tail = lub_heap_block__get_tail(block);
                    
                    lub_heap_init_free_block(this,
                                             &tail[1-delta],
                                             (delta << 2),
                                             BOOL_FALSE,
                                             tail->segment);

                    block->alloc.tag.words = words;
                    tail = lub_heap_block__get_tail(block);
                    tail->words   = words;
                    tail->free    = 0;
                    tail->segment = 0;
                    this->stats.alloc_bytes -= (delta << 2);
                 }
                break;
            }
            else if(words)
            {
                /* can we simple extend the current allocated block? */
                if(    (BOOL_TRUE == lub_heap_extend_upwards  (this,&block,words))
                    || (BOOL_TRUE == lub_heap_extend_downwards(this,&block,words))
                    || (BOOL_TRUE == lub_heap_extend_both_ways(this,&block,words)) )
                {
                    result  = LUB_HEAP_OK;
                    new_ptr = (char*)block->alloc.memory;
                    break;
                }
            }
        }
        if(words && (LUB_HEAP_FAILED == result))
        {
            /* need to reallocate and copy the data */
            new_ptr = lub_heap_new_alloc_block(this,words);
            if(NULL != new_ptr)
            {
                result = LUB_HEAP_OK;

                if(NULL != block)
                {
                    /* now copy the old contents across */
                    memcpy(new_ptr,
                           (const char*)block->alloc.memory,
                           (block->alloc.tag.words << 2) - sizeof(lub_heap_alloc_block_t));
                }
            }
            else
            {
                /* couldn't find the space for the request */
                break;
            }
        }
        /* now it's time to release the memory of the old block */
        if(NULL != block)
        {
            words_t old_words = block->alloc.tag.words;
            bool_t  done      = BOOL_FALSE;

            /* combine with the next block */
            if(LUB_HEAP_OK == lub_heap_merge_with_next(this,block))
            {
                done = BOOL_TRUE;
            }
            /* combine with the previous block */
            if(LUB_HEAP_OK == lub_heap_merge_with_previous(this,block))
            {
                done = BOOL_TRUE;
            }
            if(BOOL_FALSE == done)
            {
                /* create a new free block */
                lub_heap_new_free_block(this,block);
            }
            result = LUB_HEAP_OK;
            
            --this->stats.alloc_blocks;
            this->stats.alloc_bytes    -= (old_words << 2);
            this->stats.alloc_bytes    += sizeof(lub_heap_alloc_block_t);
            this->stats.alloc_overhead -= sizeof(lub_heap_alloc_block_t);
        }
    } while(0);

    if(LUB_HEAP_OK == result)
    {
        unsigned delta;
        
        /* align the block as required */
        if(LUB_HEAP_ALIGN_NATIVE != alignment)
        {
            lub_heap_align_block(this,
                                 alignment,
                                 &new_ptr,
                                 &size);
        }
        /* revert to client space available */
        size -= sizeof(lub_heap_alloc_block_t);
        *ptr = new_ptr;
        delta = (size - requested_size);
        
        if(*ptr && requested_size && (0 < delta))
        {
            /* make sure that any excess memory is tainted */
            lub_heap_taint_memory(*ptr + requested_size,LUB_HEAP_TAINT_ALLOC,delta);
        }
    }
    else if(   (LUB_HEAP_FAILED == result) 
            && (0               == requested_size)
            && (NULL            == *ptr))
    {
        /* freeing zero is always OK */
        result = LUB_HEAP_OK;
    }
    return result;
}
/*--------------------------------------------------------- */
