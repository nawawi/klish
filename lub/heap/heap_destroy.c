/*
 * heap_destroy.c
 */
#include "private.h"
#include "node.h"
#include "context.h"

/*--------------------------------------------------------- */
static void
_lub_heap_segment_remove(void    *ptr,
                         unsigned index,
                         size_t   size,
                         void    *arg)
{
    lub_heap_segment_t *segment = ptr;
    lub_heap_leak_t    *leak    = lub_heap_leak_instance();
    /* remove segment from the leak tree */
    lub_bintree_remove(&leak->m_segment_tree,segment);
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */
static void
_lub_heap_node_remove(lub_heap_node_t *node,
                      void            *arg)
{
    lub_heap_t         *this    = arg;
    lub_heap_context_t *context = lub_heap_node__get_context(node);
    lub_heap_leak_t    *leak    = lub_heap_leak_instance();
    if(context && (context->heap == this))
    {
        --leak->m_stats.allocs;
        leak->m_stats.alloc_bytes -= lub_heap_node__get_size(node);
        /* remove this node from the leak tree */
        lub_heap_node_fini(node);
    }
    lub_heap_leak_release(leak);
}
/*--------------------------------------------------------- */
void
lub_heap_destroy(lub_heap_t *this)
{   
    lub_heap_leak_t *leak = lub_heap_leak_instance();
    lub_heap_t     **ptr;
    for(ptr = &leak->m_heap_list;
        *ptr;
        ptr = &(*ptr)->next)
    {
        if((*ptr) == this)
        {
            /* remove from the linked list */
            *ptr = this->next;
            break;
        }
    }
    lub_heap_leak_release(leak);
    
    /* remove all segments from the leak tree */
    lub_heap_foreach_segment(this,_lub_heap_segment_remove,0);

    /* remove all nodes from the leak tree */
    lub_heap_foreach_node(_lub_heap_node_remove,this);
}
/*--------------------------------------------------------- */
