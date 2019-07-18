/** Defines an AVL tree for keeping track of verified requested resources
 * to support call stack caching in the kernel.
 *
 *@author Marcela S. Melara
 */

#ifndef __PYR_POL_AVL_TREE_H
#define __PYR_POL_AVL_TREE_H

#include <stdio.h>
#include <stdbool.h>

typedef struct pol_node
{
    char *verified_resource;
    struct pol_node*  left;
    struct pol_node*  right;
    int level;
} pol_avl_node_t;


void free_pol_avl_tree(pol_avl_node_t** root);
bool contains_resource(const char *, pol_avl_node_t *n);
pol_avl_node_t *insert_resource(const char *, pol_avl_node_t *n);

#endif /* __PYR_POL_AVL_TREE_H */
