/* Implements the wrappers around optimized system calls that support
 * verified call stack logging in Pyronia.
 *
 *@author Marcela S. Melara
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>

#include "stack_log.h"

typedef int (*real_open_t)(const char *, int);

int real_open(const char *pathname, int flags) {
    return ((real_open_t)dlsym(RTLD_NEXT, "open"))(pathname, flags);
}

// wrapper around basic open that
int open(const char *pathname, int flags) {
    int fd = -1;
    int err = -1;
    char *path_with_stack = NULL;
    bool is_logged = false;

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (is_logged) {
        err = attach_resource_req_callstack(pathname, &path_with_stack);
    }

    if (!err && path_with_stack) {
        fd = real_open(path_with_stack, flags);
    }
    else {
        // normal open
        fd = real_open(pathname, flags);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname

  return fd;
}
