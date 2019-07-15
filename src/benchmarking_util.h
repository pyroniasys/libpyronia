/* Defines the benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#ifndef PYRONIA_BENCH_H
#define PYRONIA_BENCH_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
    void init_userspace_benchmarking(void);
    void get_cpu_time(struct timespec *);
    void record_malloc(struct timespec, struct timespec);
    void record_free(struct timespec, struct timespec);
    void record_grant(struct timespec, struct timespec);
    void record_revoke(struct timespec, struct timespec);
    void record_priv_add(struct timespec, struct timespec);
    void record_priv_del(struct timespec, struct timespec);
    void record_memdom_creat(struct timespec, struct timespec);
    void record_callgraph_gen(struct timespec, struct timespec);
    void output_userspace_bench(void);

    void init_memdom_benchmarking(void);
    void set_cur_dom_label(char *);
    char *get_cur_dom_label(void);
    void record_internal_malloc(struct timespec); //struct timespec is size_t
    void record_internal_free(struct timespec);
    void record_domain_metadata_malloc(const char *, struct timespec);
    void record_domain_metadata_free(const char *, struct timespec);
    void record_new_domain_page(const char *);
    void output_memdom_bench(void);
#ifdef __cplusplus
}
#endif

#endif /* PYRONIA_BENCH_H */
