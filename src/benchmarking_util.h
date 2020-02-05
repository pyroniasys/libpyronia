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
 * Defines the benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#ifndef PYRONIA_BENCH_H
#define PYRONIA_BENCH_H

#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
    void init_userspace_benchmarking(void);
    void get_cpu_time(struct timespec *);
    void record_init(struct timespec, struct timespec);
    void record_malloc(struct timespec, struct timespec);
    void record_free(struct timespec, struct timespec);
    void record_grant(struct timespec, struct timespec);
    void record_revoke(struct timespec, struct timespec);
    void record_priv_add(struct timespec, struct timespec);
    void record_priv_del(struct timespec, struct timespec);
    void record_memdom_creat(struct timespec, struct timespec);
    void record_callstack_gen(struct timespec, struct timespec);
    void record_callstack_hash(struct timespec, struct timespec);
    void output_userspace_bench(FILE *f);

    void init_user_syscall_benchmarking(void);
    void record_open(struct timespec start, struct timespec stop);
    void record_fopen(struct timespec start, struct timespec stop);
    void record_connect(struct timespec start, struct timespec stop);
    void record_bind(struct timespec start, struct timespec stop);
    void output_user_syscall_bench(FILE *f);
#ifdef __cplusplus
}
#endif

#endif /* PYRONIA_BENCH_H */
