/*
 * heap_foreach_segment.c
 */
#include "private.h"
/*--------------------------------------------------------- */
void
lub_heap_foreach_segment(lub_heap_t          *this,
                         lub_heap_foreach_fn *fn,
                         void                *arg)
{
    lub_heap_segment_t *segment;
    unsigned int        i = 1;
    
    for(segment = &this->first_segment;
        segment;
        segment = segment->next)
    {
        /* call the client function */
        fn(segment,
           i++,
           (segment->words << 2),
           arg);
    }
}
/*--------------------------------------------------------- */
