/* Defines the userspace component of the in-kernel call stack
 * logging optimization.
 *
 *@author Marcela S. Melara
 */

#ifndef PYR_STACK_LOG_H
#define PYR_STACK_LOG_H

#include <stdbool.h>

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
bool check_verified_resource(const char *resource);
int compute_callstack_hash(unsigned char **callstack_hash);

#endif /* PYR_STACK_LOG_H */
