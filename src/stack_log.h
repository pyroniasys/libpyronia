/* Defines the userspace component of the in-kernel call stack
 * logging optimization.
 *
 *@author Marcela S. Melara
 */

#ifndef PYR_STACK_LOG_H
#define PYR_STACK_LOG_H

#include <stdbool.h>

int record_verified_resource(const char *resource);
bool check_verified_resource(const char *resource);
int compute_callstack_hash(const char *resource,
			   unsigned char **callstack_hash);

#endif /* PYR_STACK_LOG_H */
