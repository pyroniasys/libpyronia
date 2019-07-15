/* Implements the benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "benchmarking_util.h"

struct userspace_bench {
    int num_mallocs;
    double total_malloc_us;
    int num_frees;
    double total_free_us;
    int num_grants;
    double total_grant_us;
    int num_revokes;
    double total_revoke_us;
    int num_priv_adds;
    double total_priv_add_us;
    int num_priv_dels;
    double total_priv_del_us;
    int num_memdom_creats;
    double total_memdom_creat_us;
    int num_callstack_gens;
    double total_callstack_gen_us;
};

struct userspace_bench *pyr_ops_data;

void init_userspace_benchmarking(void) {

    pyr_ops_data = malloc(sizeof(struct userspace_bench));
    if (pyr_ops_data == NULL) {
        printf("Not benchmarking pyronia ops!\n");
        return;
    }
    memset(pyr_ops_data, 0, sizeof(struct userspace_bench));
}

void get_cpu_time(struct timespec *t) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, t);
}

static inline double calc_time_diff_us(struct timespec start, struct timespec stop) {
    return ((stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3);
}

void record_malloc(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_malloc_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_mallocs++;
}

void record_free(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_free_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_frees++;
}

void record_grant(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_grant_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_grants++;
}

void record_revoke(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_revoke_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_revokes++;
}

void record_priv_add(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_priv_add_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_priv_adds++;
}

void record_priv_del(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_priv_del_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_priv_dels++;
}

void record_memdom_creat(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_memdom_creat_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_memdom_creats++;
}

void record_callstack_gen(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_callstack_gen_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_callstack_gens++;
}

void output_userspace_bench() {

    if (pyr_ops_data == NULL) {
        return;
    }

    printf("%d %ld,%d %ld,%d %ld,%d %ld,%d %ld,%d %ld,%d %ld,%d %ld\n",
           pyr_ops_data->num_mallocs, pyr_ops_data->total_malloc_us,
           pyr_ops_data->num_frees, pyr_ops_data->total_free_us,
           pyr_ops_data->num_grants, pyr_ops_data->total_grant_us,
           pyr_ops_data->num_revokes, pyr_ops_data->total_revoke_us,
           pyr_ops_data->num_priv_adds, pyr_ops_data->total_priv_add_us,
           pyr_ops_data->num_priv_dels, pyr_ops_data->total_priv_del_us,
           pyr_ops_data->num_memdom_creat,
           pyr_ops_data->total_memdom_creat_us,
           pyr_ops_data->num_callgraph_gens,
           pyr_ops_data->total_callgraph_gen_us
           );

    free(pyr_ops_data);
}
