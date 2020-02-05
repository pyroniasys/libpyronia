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
 * Implements the wrappers around optimized system calls that support
 * verified call stack logging in Pyronia.
 *
 *@author Marcela S. Melara
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>

#include <smv_lib.h>
#include "stack_log.h"

#ifdef PYR_SYSCALL_BENCH
#include "benchmarking_util.h"
#endif

typedef int(*real_open_t)(const char *, int, mode_t);
typedef FILE *(*real_fopen_t)(const char *, const char *);
typedef int(*real_connect_t)(int, const struct sockaddr *, socklen_t);
typedef int(*real_bind_t)(int, const struct sockaddr *, socklen_t);

static void print_hash(unsigned char *hash_buf) {
  int i = 0;
  printf("[%s] ", __func__);
  for (i = 0; i < 32; i++) {
    printf("%02x", hash_buf[i]);
  }
  printf("\n");
}

int real_open(const char *pathname, int flags, mode_t mode) {
  return ((real_open_t)dlsym(RTLD_NEXT, "open"))(pathname, flags, mode);
}

int open(const char *pathname, int flags, mode_t mode) {
    int fd = -1;
#ifdef PYR_SYSCALL_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif
#ifdef WITH_STACK_LOGGING
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    struct pyr_userspace_stack_hash uh;

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    uh.resource.filename = pathname;
    uh.includes_stack = 0;
    if (!err && hash) {
      //print_hash(hash);
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
    }

    rlog("[%s] Passing call stack %p for %s? %s\n", __func__, &uh, uh.resource.filename, (uh.includes_stack ? "yes" : "no"));

    fd = real_open((const char *)&uh, flags, mode);
    if (fd != -1) {
      err = record_verified_resource(pathname);
      rlog("[%s] Syscall successful for path %s\n", __func__, pathname);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
#else
    fd = real_open(pathname, flags, mode);
#endif
#ifdef PYR_SYSCALL_BENCH
    get_cpu_time(&stop);
    record_open(start, stop);
#endif
  return fd;
}

int real_open64(const char *pathname, int flags, mode_t mode) {
  return ((real_open_t)dlsym(RTLD_NEXT, "open64"))(pathname, flags, mode);
}

int open64(const char *pathname, int flags, mode_t mode) {
    int fd = -1;
#ifdef PYR_SYSCALL_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif
#ifdef WITH_STACK_LOGGING
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    struct pyr_userspace_stack_hash uh;

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    uh.resource.filename = pathname;
    uh.includes_stack = 0;
    if (!err && hash) {
      //print_hash(hash);
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
    }

    rlog("[%s] Passing call stack %p for %s? %s\n", __func__, &uh, uh.resource.filename, (uh.includes_stack ? "yes" : "no"));

    fd = real_open64((const char *)&uh, flags, mode);
    if (fd != -1) {
      err = record_verified_resource(pathname);
      rlog("[%s] Syscall successful for path %s\n", __func__, pathname);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
#else
    fd = real_open64(pathname, flags, mode);
#endif
#ifdef PYR_SYSCALL_BENCH
    get_cpu_time(&stop);
    record_open(start, stop);
#endif
  return fd;
}

FILE *real_fopen(const char *pathname, const char *mode) {
  return ((real_fopen_t)dlsym(RTLD_NEXT, "fopen"))(pathname, mode);
}

// wrapper around basic open
FILE *fopen(const char *pathname, const char *mode) {
    FILE *f = NULL;
#ifdef PYR_SYSCALL_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif
#ifdef WITH_STACK_LOGGING
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    struct pyr_userspace_stack_hash uh;

    rlog("[%s] pathname: %s\n", __func__, pathname);

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    uh.resource.filename = pathname;
    uh.includes_stack = 0;
    if (!err && hash) {
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
        //print_hash(uh.hash);
    }

    rlog("[%s] Passing call stack %p for %s? %s\n", __func__, &uh, uh.resource.filename, (uh.includes_stack ? "yes" : "no"));
    f = real_fopen((const char *)&uh, mode);

    // syscall succeeded, so record the resource as verified
    if (f != NULL) {
      err = record_verified_resource(pathname);
    }
    else {
      //printf("Failed to open %s: %s\n", pathname, strerror(errno));
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
#else
    f = real_fopen(pathname, mode);
#endif
#ifdef PYR_SYSCALL_BENCH
    get_cpu_time(&stop);
    record_fopen(start, stop);
#endif
  return f;
}

FILE *real_fopen64(const char *pathname, const char *mode) {
  return ((real_fopen_t)dlsym(RTLD_NEXT, "fopen64"))(pathname, mode);
}

// wrapper around basic open
FILE *fopen64(const char *pathname, const char *mode) {
    FILE *f = NULL;
#ifdef PYR_SYSCALL_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif
#ifdef WITH_STACK_LOGGING
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    struct pyr_userspace_stack_hash uh;

    rlog("[%s] pathname: %s\n", __func__, pathname);

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    uh.resource.filename = pathname;
    uh.includes_stack = 0;
    if (!err && hash) {
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
        //print_hash(uh.hash);
    }

    rlog("[%s] Passing call stack %p for %s? %s\n", __func__, &uh, uh.resource.filename, (uh.includes_stack ? "yes" : "no"));
    f = real_fopen64((const char *)&uh, mode);

    // syscall succeeded, so record the resource as verified
    if (f != NULL) {
      err = record_verified_resource(pathname);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
#else
    f = real_fopen64(pathname, mode);
#endif
#ifdef PYR_SYSCALL_BENCH
    get_cpu_time(&stop);
    record_fopen(start, stop);
#endif
  return f;
}

// TODO: move this to uapi/linux/pyronia.h
static void in_addr_to_str(const struct sockaddr *sa, char *addr_str)
{
    int in_addr = 0, printed_bytes = 0;
    char ip_str[INET_ADDRSTRLEN+1];
    char ip6_str[INET6_ADDRSTRLEN+1];

    // TODO: we might get a junk address, so fail gracefully

    if (sa->sa_family == AF_INET) {
        in_addr = (int)((struct sockaddr_in*)sa)->sin_addr.s_addr;

        memset(ip_str, 0, INET_ADDRSTRLEN+1);
        printed_bytes = snprintf(ip_str, INET_ADDRSTRLEN, "%d.%d.%d.%d",
                                 (in_addr & 0xFF),
                                 ((in_addr & 0xFF00) >> 8),
                                 ((in_addr & 0xFF0000) >> 16),
                                 ((in_addr & 0xFF000000) >> 24));

        if (printed_bytes > sizeof(ip_str)) {
            return;
        }

        strcpy(addr_str, ip_str);
    }
    else if (sa->sa_family == AF_INET6) {
        // Support IPv6 addresses
        unsigned char *s6_addrp = ((struct sockaddr_in6*)sa)->sin6_addr.s6_addr;
        memset(ip6_str, 0, INET6_ADDRSTRLEN+1);
        printed_bytes = snprintf(ip6_str, INET6_ADDRSTRLEN, "%x:%x:%x:%x:%x:%x:%x:%x",
                                 (s6_addrp[0] | s6_addrp[1]),
                                 (s6_addrp[2] | s6_addrp[3]),
                                 (s6_addrp[4] | s6_addrp[5]),
                                 (s6_addrp[6] | s6_addrp[7]),
                                 (s6_addrp[8] | s6_addrp[9]),
                                 (s6_addrp[10] | s6_addrp[11]),
                                 (s6_addrp[12] | s6_addrp[13]),
                                 (s6_addrp[14] | s6_addrp[15]));
        if (printed_bytes > sizeof(ip6_str)) {
            return;
        }

        strcpy(addr_str, ip6_str);
    }
    else if (sa->sa_family == AF_NETLINK) {
      strcpy(addr_str, "netlink\0");
    }
    else {
      strcpy(addr_str, "unknown\0");
    }
}

int real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return ((real_connect_t)dlsym(RTLD_NEXT, "connect"))(sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int err = -1;
#ifdef PYR_SYSCALL_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif
#ifdef WITH_STACK_LOGGING
    unsigned char *hash = NULL;
    bool is_logged = false;
    char addr_str[INET6_ADDRSTRLEN+1];
    struct pyr_userspace_stack_hash uh;

    memset(addr_str, 0, INET6_ADDRSTRLEN+1);
    in_addr_to_str(addr, addr_str);
    if (addr_str[0] == '\0') {
      errno = EADDRNOTAVAIL; // is this the best errno for this case?
      goto out;
    }

    // check to see if the requested IP address has already been verified
    is_logged = check_verified_resource(addr_str);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    rlog("[%s] Got sockaddr for %s\n", __func__, addr_str);

    uh.resource.addr = addr;
    uh.includes_stack = 0;
    if (!err && hash) {
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
        //print_hash(uh.hash);
    }
    rlog("[%s] Passing call stack for %s? %s\n", __func__, addr_str, (uh.includes_stack ? "yes" : "no"));

    err = real_connect(sockfd, (const struct sockaddr *)&uh, addrlen);

    // syscall succeeded, so record the resource as verified
    if (err == 0) {
      err = record_verified_resource(addr_str);
      rlog("[%s] syscall succeeded\n", __func__);
    }

 out:
    memset(addr_str, 0, INET6_ADDRSTRLEN+1);
    if (hash)
      free(hash);
#else
    err = real_connect(sockfd, addr, addrlen);
#endif
#ifdef PYR_SYSCALL_BENCH
    get_cpu_time(&stop);
    record_connect(start, stop);
#endif
  return err;
}

/*
int real_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return ((real_bind_t)dlsym(RTLD_NEXT, "bind"))(sockfd, addr, addrlen);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    char addr_str[INET6_ADDRSTRLEN+1];
    struct pyr_userspace_stack_hash uh;

    memset(addr_str, 0, INET6_ADDRSTRLEN+1);
    in_addr_to_str(addr, addr_str);
    if (addr_str[0] == '\0') {
      errno = EADDRNOTAVAIL; // is this the best errno for this case?
      goto out;
    }

    // check to see if the requested IP address has already been verified
    is_logged = check_verified_resource(addr_str);
    if (is_logged) {
        err = compute_callstack_hash(&hash);
    }

    printf("[%s] Got sockaddr for %s\n", __func__, addr_str);

    if (!strcmp(addr_str, "netlink")) {
      // SMV and Pyronia use netlink socket binds before we've actually
      // initialized the subsystem
      err = real_bind(sockfd, addr, addrlen);
    }
    else {
      uh.resource.addr = addr;
      uh.includes_stack = 0;
      if (!err && hash) {
        //print_hash(hash);
        uh.includes_stack = 1;
        memcpy(uh.hash, hash, SHA256_DIGEST_SIZE);
      }

      printf("[%s] Passing call stack for %s? %s\n", __func__, addr_str, (uh.includes_stack ? "yes" : "no"));

      err = real_bind(sockfd, (const struct sockaddr *)&uh, addrlen);

      // syscall succeeded, so record the resource as verified
      if (err == 0) {
        err = record_verified_resource(addr_str);
        rlog("[%s] syscall succeeded\n", __func__);
      }
      else {
        perror("[bind]");
      }
    }
 out:
    memset(addr_str, 0, INET6_ADDRSTRLEN+1);
    if (hash)
      free(hash);
    return err;
}
*/
