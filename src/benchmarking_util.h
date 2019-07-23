/* Defines the benchmarking utilities.
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

    void init_memdom_benchmarking(void);
    void record_internal_malloc(size_t sz);
    void record_internal_free(size_t sz);
    void record_memdom_metadata_alloc(size_t sz);
    void record_memdom_metadata_free(size_t sz);
    void record_new_domain_page(void);
    void output_memdom_bench(const char *);
#ifdef __cplusplus
}
#endif

#endif /* PYRONIA_BENCH_H */
