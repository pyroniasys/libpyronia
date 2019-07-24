/* Implements the benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "benchmarking_util.h"

struct userspace_bench {
    int num_inits;
    double total_init_us;
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
    int num_callstack_hashes;
    double total_callstack_hash_us;
};

struct userspace_bench *pyr_ops_data = NULL;

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

void record_init(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_init_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_inits++;
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

void record_callstack_hash(struct timespec start, struct timespec stop) {
    pyr_ops_data->total_callstack_hash_us += calc_time_diff_us(start, stop);
    pyr_ops_data->num_callstack_hashes++;
}

void output_userspace_bench(FILE *f) {

    if (pyr_ops_data == NULL) {
        return;
    }

    fprintf(f, "%d %.2f,%d %.2f,%d %.2f,%d %.2f,%d %.2f,%d %.2f,%d %.2f,%d %.2f,%d %.2f\n",
           pyr_ops_data->num_mallocs, pyr_ops_data->total_malloc_us,
           pyr_ops_data->num_frees, pyr_ops_data->total_free_us,
           pyr_ops_data->num_grants, pyr_ops_data->total_grant_us,
           pyr_ops_data->num_revokes, pyr_ops_data->total_revoke_us,
           pyr_ops_data->num_priv_adds, pyr_ops_data->total_priv_add_us,
           pyr_ops_data->num_priv_dels, pyr_ops_data->total_priv_del_us,
           pyr_ops_data->num_memdom_creats,
           pyr_ops_data->total_memdom_creat_us,
           pyr_ops_data->num_callstack_gens,
           pyr_ops_data->total_callstack_gen_us,
           pyr_ops_data->num_callstack_hashes,
           pyr_ops_data->total_callstack_hash_us
           );

    free(pyr_ops_data);
}

struct user_syscall_bench {
    int num_opens;
    double total_open_us;
    int num_fopens;
    double total_fopen_us;
    int num_connects;
    double total_connect_us;
    int num_binds;
    double total_bind_us;
};

struct user_syscall_bench *pyr_syscall_data = NULL;

void init_user_syscall_benchmarking(void) {
    pyr_syscall_data = malloc(sizeof(struct user_syscall_bench));
    if (pyr_syscall_data == NULL) {
        printf("Not benchmarking syscalls ops!\n");
        return;
    }
    memset(pyr_syscall_data, 0, sizeof(struct user_syscall_bench));
}

void record_open(struct timespec start, struct timespec stop) {
    if (!pyr_syscall_data)
      return;
    pyr_syscall_data->total_open_us += calc_time_diff_us(start, stop);
    pyr_syscall_data->num_opens++;
}

void record_fopen(struct timespec start, struct timespec stop) {
    if (!pyr_syscall_data)
      return;
    pyr_syscall_data->total_fopen_us += calc_time_diff_us(start, stop);
    pyr_syscall_data->num_fopens++;
}

void record_connect(struct timespec start, struct timespec stop) {
    if (!pyr_syscall_data)
      return;
    pyr_syscall_data->total_connect_us += calc_time_diff_us(start, stop);
    pyr_syscall_data->num_connects++;
}

void record_bind(struct timespec start, struct timespec stop) {
    if (!pyr_syscall_data)
      return;
    pyr_syscall_data->total_bind_us += calc_time_diff_us(start, stop);
    pyr_syscall_data->num_binds++;
}

void output_user_syscall_bench(FILE *f) {

    if (pyr_syscall_data == NULL) {
        return;
    }

    fprintf(f, "%d %.2f,%d %.2f,%d %.2f\n",
           pyr_syscall_data->num_opens, pyr_syscall_data->total_open_us,
           pyr_syscall_data->num_fopens, pyr_syscall_data->total_fopen_us,
           pyr_syscall_data->num_connects, pyr_syscall_data->total_connect_us
           );

    free(pyr_syscall_data);
}
