#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
void
lub_heap_block_getkey(const void        *clientnode,
                      lub_bintree_key_t *key)
{
    const lub_heap_free_block_t *block = clientnode;
    lub_heap_key_t               mykey;
    
    mykey.words = block->tag.words;
    mykey.block = block;
    
    memcpy(key,&mykey,sizeof(lub_heap_key_t));
}
/*--------------------------------------------------------- */
