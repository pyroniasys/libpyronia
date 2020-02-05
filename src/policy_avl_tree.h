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
 * Defines an AVL tree for keeping track of verified requested resources
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
