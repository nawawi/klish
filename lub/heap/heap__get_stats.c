#include "private.h"
/*--------------------------------------------------------- */
void 
lub_heap__get_stats(lub_heap_t       *this,
                    lub_heap_stats_t *stats)
{
    *stats = this->stats;
}
/*--------------------------------------------------------- */
