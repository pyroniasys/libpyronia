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
 * Contains the Pyronia kernel communication library used for
 * receiving callstack requests and sending back serialized callgraphs.
 *
 *@author Marcela S. Melara
 */

#ifndef __PYR_KERNEL_COMM_H
#define __PYR_KERNEL_COMM_H

#include <stdbool.h>

#define FAMILY_STR "SI_COMM"

// let's place these here since they're really
// only shared between the main library API and the
// SI comm channel
pthread_mutex_t security_ctx_mutex;
pthread_cond_t si_cond_var;
bool is_inspecting_stack;

#ifdef __cplusplus
extern "C" {
#endif

  int pyr_init_si_comm(char *policy, bool is_child);
    extern int pyr_collect_runtime_callstack(void);
    void pyr_teardown_si_comm(void);
    void *pyr_recv_from_kernel(void *args);
    int pyr_callstack_req_listen(bool is_child);
    void pyr_is_inspecting(void);

#ifdef __cplusplus
}
#endif

#endif
