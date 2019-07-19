/* Implements the wrappers around optimized system calls that support
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
#include <errno.h>
#include "stack_log.h"

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

/*
int real_open(const char *pathname, int flags, mode_t mode) {
  return ((real_open_t)dlsym(RTLD_NEXT, "open"))(pathname, flags, mode);
}

int open(const char *pathname, int flags, mode_t mode) {
    int fd = -1;
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (!is_logged) {
        err = compute_callstack_hash(pathname, &hash);
    }

    if (!err && hash) {
      //print_hash(hash);
        fd = real_open(pathname, flags, mode);
    }
    else {
        // normal open
        fd = real_open(pathname, flags, mode);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
  return fd;
  }*/

FILE *real_fopen(const char *pathname, const char *mode) {
  return ((real_fopen_t)dlsym(RTLD_NEXT, "fopen"))(pathname, mode);
}

// wrapper around basic open
FILE *fopen(const char *pathname, const char *mode) {
    FILE *f = NULL;
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;

    // check to see if the requested pathname has already been verified
    is_logged = check_verified_resource(pathname);
    if (!is_logged) {
        err = compute_callstack_hash(pathname, &hash);
    }

    if (!err && hash) {
        print_hash(hash);
        f = real_fopen(pathname, mode);
    }
    else {
        // normal open
        f = real_fopen(pathname, mode);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
    if (hash)
      free(hash);
  return f;
}

// TODO: move this to uapi/linux/pyronia.h
static void in_addr_to_str(const struct sockaddr *sa, const char**addr_str)
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
            *addr_str = NULL;
            return;
        }

        *addr_str = ip_str;
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
            *addr_str = NULL;
            return;
        }

        *addr_str = ip6_str;
    }
    else if (sa->sa_family == AF_NETLINK) {
      *addr_str  = "netlink";
    }
    else {
      *addr_str  = "unknown";
    }
}

int real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return ((real_connect_t)dlsym(RTLD_NEXT, "connect"))(sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int err = -1;
    unsigned char *hash = NULL;
    bool is_logged = false;
    const char *addr_str = NULL;

    in_addr_to_str(addr, &addr_str);
    if (addr_str == NULL) {
      errno = EADDRNOTAVAIL; // is this the best errno for this case?
      goto out;
    }
    
    // check to see if the requested IP address has already been verified
    is_logged = check_verified_resource(addr_str);
    if (!is_logged) {
        err = compute_callstack_hash(addr_str, &hash);
    }

    if (!err && hash) {
        print_hash(hash);
        err = real_connect(sockfd, addr, addrlen);
    }
    else {
        // normal connect
        err = real_connect(sockfd, addr, addrlen);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
 out:
    if (hash)
      free(hash);
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
    const char *addr_str = NULL;

    in_addr_to_str(addr, &addr_str);
    if (addr_str == NULL) {
      errno = EADDRNOTAVAIL; // is this the best errno for this case?
      goto out;
    }
    
    // check to see if the requested IP address has already been verified
    is_logged = check_verified_resource(addr_str);
    if (!is_logged) {
        err = compute_callstack_hash(addr_str, &hash);
    }

    if (!err && hash) {
      //print_hash(hash);
        err = real_bind(sockfd, addr, addrlen);
    }
    else {
        // normal bind
        err = real_bind(sockfd, addr, addrlen);
    }

    // the SI thread will take care of logging if the kernel
    // verifies the stack for this pathname
 out:
    if (err)
      printf("[%s] Error %s\n", __func__, strerror(errno));
    return err;
}
*/
