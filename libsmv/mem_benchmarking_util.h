/* Defines the memory usage benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#ifndef PYRONIA_MEM_BENCH_H
#define PYRONIA_MEM_BENCH_H

#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
    void init_memdom_benchmarking(void);
    void record_internal_malloc(size_t sz);
    void record_internal_free(size_t sz);
    void record_memdom_metadata_alloc(size_t sz);
    void record_memdom_metadata_free(size_t sz);
    void record_new_domain_page(void);
    void output_memdom_bench(FILE *);
#ifdef __cplusplus
}
#endif

#endif /* PYRONIA_MEM_BENCH_H */
