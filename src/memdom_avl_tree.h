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
 * Defines an AVL tree for managing the memdom metadata
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
