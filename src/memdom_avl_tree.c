/* Copyright 2019 Princeton University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Implements an AVL tree for managing the memdom metadata
 * more efficiently.
 *
 *@author Marcela S. Melara
 */

#include <stdio.h>
#include <stdlib.h>
#include <memdom_lib.h>
#include "memdom_avl_tree.h"
#include "security_context.h"

/* Free all nodes in the given AVL tree
 */
void free_avl_tree(avl_node_t** root) {
    avl_node_t *rootp = *root;

    if (!rootp)
      return;

    if (rootp->left != NULL)
        free_avl_tree(&rootp->left);
    if (rootp->right != NULL)
        free_avl_tree(&rootp->right);
    free_interp_dom_metadata(&rootp->memdom_metadata);

#ifdef MEMDOM_BENCH
    record_memdom_metadata_free(sizeof(avl_node_t));
#endif
    free(rootp);
    *root = NULL;
}

/* Search the tree for the node with the interp_dom_alloc metadata
 * containing the given addr.
 */
pyr_interp_dom_alloc_t* find_memdom_metadata(void *addr, avl_node_t *n) {
    pyr_interp_dom_alloc_t *metadata = NULL;
    if (n == NULL) {
        return NULL;
    }

    metadata = n->memdom_metadata;
    if (!metadata)
        return NULL;

    if (addr < metadata->start)
        return find_memdom_metadata(addr, n->left);
    else if (addr > metadata->end)
        return find_memdom_metadata(addr, n->right);
    else if (addr >= metadata->start && addr < metadata->end)
        return metadata;
    else if (addr == metadata->end)
        return find_memdom_metadata(addr, n->right);

    return NULL;
}

pyr_interp_dom_alloc_t *find_memdom_with_space(avl_node_t *n, size_t size) {
    pyr_interp_dom_alloc_t *metadata = NULL;
    if (n == NULL) {
        return NULL;
    }

    metadata = n->memdom_metadata;
    if (!metadata)
        return NULL;

    if (metadata->has_space &&
        memdom_get_free_bytes(metadata->memdom_id) >= size) {
        return metadata;
    }

    metadata = find_memdom_with_space(n->left, size);

    if (!metadata)
        metadata = find_memdom_with_space(n->right, size);

    return metadata;
}

/* Get the level of a node
 */
static inline int get_level(avl_node_t *n) {
    if (n == NULL)
        return -1;
    else
        return n->level;
}

/* Get the larger of two integers
 */
static inline int max(int l, int r)
{
    return l > r ? l : r;
}

/* Compute the balance factor of a given node */
static inline int balance_factor(avl_node_t *n) {
    return (get_level(n->left) - get_level(n->right));
}

/* Perform a right rotation between a tree node rooted at n and its left child.
 * Note: Caller must ensure that n->left is not NULL.
 */
static avl_node_t *rotate_right(avl_node_t* n) {
    avl_node_t *new_root = n->left;
    avl_node_t *new_left = new_root->right;

    // do rotation
    new_root->right = n;
    n->left = new_left;

    n->level = max(get_level(n->left), get_level(n->right)) + 1;
    new_root->level = max(get_level(new_root->left), get_level(new_root->right)) + 1;
    return new_root; /* new root */
}

/* Perform a left rotation between a tree node at n and its right child.
 * Note: Caller must ensure that n->right is not NULL.
 */
static avl_node_t *rotate_left(avl_node_t* n) {
    avl_node_t *new_root = n->right;
    avl_node_t *new_right = new_root->left;

    // do rotation
    new_root->left = n;
    n->right = new_right;

    n->level = max(get_level(n->left), get_level(n->right)) + 1;
    new_root->level = max(get_level(new_root->left), get_level(new_root->right)) + 1;

    return new_root;  /* New root */
}

/* Balances the tree performing the necessary rotations.
 * Returns the new root of the tree.
 */
static avl_node_t *balance_tree(avl_node_t *n) {
    if (balance_factor(n) < -1) {
        if (balance_factor(n->right) > 0)
            n->right = rotate_right(n->right);
        n = rotate_left(n);
    }
    else if (balance_factor(n) > 1) {
        if (balance_factor(n->left) < 0)
            n->left = rotate_left(n->left);
        n = rotate_right(n);
    }
    return n;
}

/* Inserts the given memdom metadata into the tree.
 * Performs any balancing, and returns the new root.
 */
avl_node_t *insert_memdom_metadata(pyr_interp_dom_alloc_t *metadata, avl_node_t *n) {
  
    if (n == NULL) {
        /* Create and return a one-node tree */
        n = malloc(sizeof(avl_node_t));
        if (!n) {
            fprintf (stderr, "Out of memory!!! (insert)\n");
            return NULL;
        }
#ifdef MEMDOM_BENCH
        record_memdom_metadata_alloc(sizeof(avl_node_t));
#endif
        n->memdom_metadata = metadata;
        n->level = 1;
        n->left = n->right = NULL;
        return n;
    }
    else if (metadata->start <  n->memdom_metadata->start) {
        n->left = insert_memdom_metadata(metadata, n->left);
    }
    else if(metadata->start > n->memdom_metadata->start) {
        n->right = insert_memdom_metadata(metadata, n->right);
    }
    n->level = max(get_level(n->left), get_level(n->right)) + 1;
    return balance_tree(n);
}
