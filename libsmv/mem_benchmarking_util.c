/* Implements the memory usage benchmarking utilities.
 *
 *@author Marcela S. Melara
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_benchmarking_util.h"

struct memdom_bench {
    size_t peak_alloc;
    size_t cur_alloc;
    size_t peak_metadata_alloc;
    size_t cur_metadata_alloc;
    int num_pages;
};

struct memdom_bench *pyr_memdom_data;

void init_memdom_benchmarking(void) {
    pyr_memdom_data = malloc(sizeof(struct memdom_bench));
    if (pyr_memdom_data == NULL) {
        printf("Not benchmarking memdom mem usage!\n");
        return;
    }
    memset(pyr_memdom_data, 0, sizeof(struct memdom_bench));
}

void record_internal_malloc(size_t sz) {
    pyr_memdom_data->cur_alloc += sz;
    if (pyr_memdom_data->cur_alloc > pyr_memdom_data->peak_alloc)
        pyr_memdom_data->peak_alloc = pyr_memdom_data->cur_alloc;
}

void record_internal_free(size_t sz) {
    pyr_memdom_data->cur_alloc -= sz;
}

void record_memdom_metadata_alloc(size_t sz) {
    pyr_memdom_data->cur_metadata_alloc += sz;
    if (pyr_memdom_data->cur_metadata_alloc > pyr_memdom_data->peak_metadata_alloc)
        pyr_memdom_data->peak_metadata_alloc = pyr_memdom_data->cur_metadata_alloc;
    record_internal_malloc(sz);
}

void record_memdom_metadata_free(size_t sz) {
    pyr_memdom_data->cur_metadata_alloc -= sz;
    record_internal_free(sz);
}

void record_new_domain_page() {
    pyr_memdom_data->num_pages++;
}

void output_memdom_bench(FILE *f) {
    if (pyr_memdom_data == NULL) {
        return;
    }

    fprintf(f, "%lu, %lu, %d\n",
            pyr_memdom_data->peak_alloc,
            pyr_memdom_data->peak_metadata_alloc,
            pyr_memdom_data->num_pages
           );
    
    free(pyr_memdom_data);
}
