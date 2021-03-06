/* Copyright 2018 Princeton University
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
 * Contains the Pyronia security context definitions used in Pyronia-aware
 * language runtimes to isolate security-critical runtime state and
 * native libraries into memory domains.
 *
 *@author Marcela S. Melara
 */

#ifndef __PYR_SEC_CTX_H
#define __PYR_SEC_CTX_H

#include <stdbool.h>

#include "memdom_avl_tree.h"
#include "policy_avl_tree.h"

#define MAX_NUM_INTERP_DOMS 1024

struct pyr_native_lib_context {
    char *library_name; // runtimes also identify libraries by string name
    int memdom_id; // the memdom this native library belongs to
    // points to the next native lib context in the linked list
    struct pyr_native_lib_context *next;
};

typedef struct pyr_native_lib_context pyr_native_ctx_t;

struct pyr_interp_dom_alloc {
    int memdom_id;
    void *start;
    void *end;
    bool has_space;
    bool writable;
    int nested_grants;
};

typedef struct pyr_interp_dom_alloc pyr_interp_dom_alloc_t;

/* A language runtime's security context:
 * Used for pyronia-related bookkeeping */
struct pyr_security_context {
    char *main_path;
    pyr_native_ctx_t *native_libs;
    avl_node_t *interp_doms;
    /* The runtime may grant write access to the critical state
     * in a function that calls another function that grants access
     * itself. To make sure we don't revoke access to the outer
     * functions, let's basically keep a reference count. */
    uint32_t nested_grants;

    pol_avl_node_t *verified_resources;
    /* The function used to collect a language runtime-specific
     * callstack. This callback needs to be set at initialization time. */
    int (*collect_callstack_cb)(void);
    void (*interpreter_lock_acquire_cb)(void);
    void (*interpreter_lock_release_cb)(void);
};

#ifdef __cplusplus
extern "C" {
#endif

    int pyr_new_native_lib_context(pyr_native_ctx_t **ctxp, const char *lib,
                                   pyr_native_ctx_t *next);
    int pyr_security_context_alloc(struct pyr_security_context **ctxp,
                                   int (*collect_callstack_cb)(void),
				   void (*interpreter_lock_acquire_cb)(void),
				   void (*interpreter_lock_release_cb)(void),
				   bool is_child);
    int pyr_find_native_lib_memdom(pyr_native_ctx_t *start, const char *lib);
    void free_interp_dom_metadata(pyr_interp_dom_alloc_t **dom);
    void pyr_security_context_free(struct pyr_security_context **ctxp);

#ifdef __cplusplus
}
#endif

#endif
