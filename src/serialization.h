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
 * Defines the API for parsing and serializing a Pyronia-secured
 * application's callstack and library-level access policies.
 *
 *@author Marcela S. Melara
 */

#ifndef PYR_SERIALIZE_H
#define PYR_SERIALIZE_H

#define INT32_STR_SIZE 12

int si_memdom;

#ifdef __cplusplus
extern "C" {
#endif

  int pyr_parse_lib_policy(const char *policy_fname, char **parsed);
  void *get_si_memdom_addr(void);
  int finalize_callstack_str(char **cs_str);

#ifdef __cplusplus
}
#endif

#endif /* PYR_SERIALIZE_H */
