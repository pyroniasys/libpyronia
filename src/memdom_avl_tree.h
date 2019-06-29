/** Defines an AVL tree for managing the memdom metadata
 * more efficiently.
 *
 *@author Marcela S. Melara
 */

#ifndef __PYR_AVL_TREE_H
#define __PYR_AVL_TREE_H

#include <stdio.h>

// forward declaration
struct pyr_interp_dom_alloc;
typedef struct pyr_interp_dom_alloc pyr_interp_dom_alloc_t;

typedef struct node
{
    pyr_interp_dom_alloc_t *memdom_metadata;
    struct node*  left;
    struct node*  right;
    int level;
} avl_node_t;


void free_avl_tree(avl_node_t** root);
pyr_interp_dom_alloc_t* find_memdom_metadata(void *addr, avl_node_t *n);
pyr_interp_dom_alloc_t *find_memdom_with_space(avl_node_t *n, size_t size);
avl_node_t *insert_memdom_metadata(pyr_interp_dom_alloc_t *metadata, avl_node_t *n);

#endif /* __PYR_AVL_TREE_H */
