/* Copyright 2018 Princeton University
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
 * Pyronia userspace API utility function definitions.
 *
 *@author Marcela S. Melara
 */

#ifndef __PYR_UTIL_H
#define __PYR_UTIL_H

#include <string.h>
#include <stdlib.h>

#include "pyronia_lib.h"

/**
 * set_str - copy the src string into the dest string
 * in a clean buffer. This is used to cleanly set the
 * string fields in the various Pyronia data structures.
 */
static inline int set_str(const char *src, char **dest) {
  char *str = NULL;
  int err = 0;

  // let's make sure to not override a non-null
  // destination
  if (*dest)
    return 0;

  str = pyr_alloc_critical_runtime_state(strlen(src)+1);
  if (!str) {
    err = -1;
    printf("[%s] Could not allocate string %s in interp dom\n", __func__, src);
    goto out;
  }

  memset(str, 0, strlen(src)+1);
  memcpy(str, src, strlen(src));

 out:
  *dest = str;
  return err;
}

#endif /* __PYR_UTIL_H */
