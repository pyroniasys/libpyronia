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
 * Defines the userspace component of the in-kernel call stack
 * logging optimization.
 *
 *@author Marcela S. Melara
 */

#ifndef PYR_STACK_LOG_H
#define PYR_STACK_LOG_H

//#define PYR_SYSCALL_BENCH

#define WITH_STACK_LOGGING // uncomment for stack logging

#define SHA256_DIGEST_SIZE 32

struct pyr_userspace_stack_hash {
    union {
        const char *filename;
        const struct sockaddr *addr;
    } resource;
    int includes_stack;
    unsigned char hash[SHA256_DIGEST_SIZE];
};

int record_verified_resource(const char *resource);
int check_verified_resource(const char *resource);
int compute_callstack_hash(unsigned char **callstack_hash);

#endif /* PYR_STACK_LOG_H */
