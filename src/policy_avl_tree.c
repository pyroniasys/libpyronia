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
 * Implements an AVL tree for keeping track of verified requested resources
 * to support call stack caching in the kernel.
 *
 *@author Marcela S. Melara
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memdom_lib.h>
#include "policy_avl_tree.h"

/* Free all nodes in the given AVL tree
 */
void free_pol_avl_tree(pol_avl_node_t** root) {
    pol_avl_node_t *rootp = *root;

    if (!rootp)
      return;

    if (rootp->left != NULL)
        free_pol_avl_tree(&rootp->left);
    if (rootp->right != NULL)
        free_pol_avl_tree(&rootp->right);
    if (rootp->verified_resource) {
#ifdef MEMDOM_BENCH
        record_internal_free(strlen(rootp->verified_resource)+1);
#endif
        free(rootp->verified_resource);
    }
#ifdef MEMDOM_BENCH
    record_internal_free(sizeof(pol_avl_node_t));
#endif
    free(rootp);
    *root = NULL;
}

/* Search the tree for the node containing the given resource.
 */
bool contains_resource(const char *resource, pol_avl_node_t *n) {
    int cmp = 0;
    if (n == NULL) {
        return false;
    }

    cmp = strcmp(n->verified_resource, resource);
    if (cmp < 0)
        return contains_resource(resource, n->left);
    else if (cmp > 0)
        return contains_resource(resource, n->right);

    return true;
}

/* Get the level of a node
 */
static inline int get_level(pol_avl_node_t *n) {
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
static inline int balance_factor(pol_avl_node_t *n) {
    return (get_level(n->left) - get_level(n->right));
}

/* Perform a right rotation between a tree node rooted at n and its left child.
 * Note: Caller must ensure that n->left is not NULL.
 */
static pol_avl_node_t *rotate_right(pol_avl_node_t* n) {
    pol_avl_node_t *new_root = n->left;
    pol_avl_node_t *new_left = new_root->right;

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
static pol_avl_node_t *rotate_left(pol_avl_node_t* n) {
    pol_avl_node_t *new_root = n->right;
    pol_avl_node_t *new_right = new_root->left;

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
static pol_avl_node_t *balance_tree(pol_avl_node_t *n) {
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

/* Inserts the given verified resource into the tree.
 * Performs any balancing, and returns the new root.
 */
pol_avl_node_t *insert_resource(const char *resource, pol_avl_node_t *n) {
    int cmp = 0;
    if (n == NULL) {
        /* Create and return a one-node tree */
        n = malloc(sizeof(pol_avl_node_t));
        if (!n) {
            fprintf (stderr, "Out of memory!!! (insert)\n");
            return NULL;
        }
#ifdef MEMDOM_BENCH
        record_internal_malloc(sizeof(pol_avl_node_t));
#endif
        n->verified_resource = malloc(strlen(resource)+1);
        if (!n->verified_resource)
            return NULL;
#ifdef MEMDOM_BENCH
        record_internal_malloc(strlen(resource)+1);
#endif
        memset(n->verified_resource, 0, strlen(resource)+1);
        memcpy(n->verified_resource, resource, strlen(resource));
        n->level = 1;
        n->left = n->right = NULL;
        return n;
    }
    else {
        cmp = strcmp(n->verified_resource, resource);
        if (cmp < 0) {
            n->left = insert_resource(resource, n->left);
        }
        else if(cmp > 0) {
            n->right = insert_resource(resource, n->right);
        }
    }
    n->level = max(get_level(n->left), get_level(n->right)) + 1;
    return balance_tree(n);
}
