/** Defines the API for parsing and serializing a Pyronia-secured
 * application's callstack and library-level access policies.
 *
 *@author Marcela S. Melara
 */

#ifndef PYR_SERIALIZE_H
#define PYR_SERIALIZE_H

#include <linux/pyronia_mac.h>

#define INT32_STR_SIZE 12

int si_memdom;

#ifdef __cplusplus
extern "C" {
#endif

  int pyr_parse_lib_policy(const char *policy_fname, char **parsed);
  int finalize_callstack_str(char **cs_str);

#ifdef __cplusplus
}
#endif

#endif /* PYR_SERIALIZE_H */
