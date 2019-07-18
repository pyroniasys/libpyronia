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
int attach_resource_req_callstack(const char *resource, char **callstack_str);

#endif /* PYR_STACK_LOG_H */
