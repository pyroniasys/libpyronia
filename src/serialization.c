/** Implements the library for parsing and serializing a Pyronia-secured
 * application's callstack and library-level access policies.
 *
 *@author Marcela S. Melara
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <error.h>
//#include <linux/pyronia.h>
#include <memdom_lib.h>

#include "pyronia_lib.h"
#include "serialization.h"

#define CALLSTACK_STR_DELIM ","

static char *serialized = NULL;
static uint32_t ser_len = 1;
static uint32_t node_count = 0;

// Serialize a callstack "object" to a tokenized string
// that the LSM can then parse. Do basic input sanitation as well.
// The function uses strncat(), which appends a string to the given
// "dest" string, so the serialized callstack is ordered from root to leaf.
// Caller must memdom_free the string.
int pyr_serialize_callstack(const char *func_fqn) {
    char *tmp_ser = NULL;
    char *delim = CALLSTACK_STR_DELIM;
    int ret = -1;

    // let's sanity check our lib name first (i.e. it should not
    // contain our delimiter character
    if (strchr(func_fqn, *delim)) {
      printf("[%s] Oops, library name %s contains unacceptable character\n", __func__, func_fqn);
      goto fail;
    }

    if (!get_si_memdom_addr())
      goto fail;

    tmp_ser = (char *) get_si_memdom_addr();
    
    if (serialized)
      // because we traverse the call stack bottom up in the runtime,
      // but we want the kernel to check it top-down, we need to
      // copy the previous frames into the end of the string
      memcpy(tmp_ser+strlen(func_fqn)+1, serialized, ser_len);
    memset(tmp_ser, 0, strlen(func_fqn));
    serialized = tmp_ser;
    
    memcpy(serialized, func_fqn, strlen(func_fqn));
    memcpy(serialized+strlen(func_fqn), CALLSTACK_STR_DELIM, 1);
    ser_len += strlen(func_fqn)+1;
    node_count++;

    rlog("[%s] Serialized node: %s, # nodes %d\n", __func__, serialized, node_count);
    ret = 0;
    goto out;

 fail:
    serialized = NULL;
    ser_len = 1;
    node_count = 0;
 out:
    return ret;
}

int finalize_callstack_str(char **cs_str) {
  int ret = -1;
  char *out = NULL;
  char tmp[1024];
  
  // now we need to pre-append the len so the kernel knows how many
  // nodes to expect to de-serialize
  if (!serialized)
    goto out;

  if (!get_si_memdom_addr())
    goto out;

  out = (char *) get_si_memdom_addr();
  if (serialized) {
    // because we traverse the call stack bottom up in the runtime,
    // but we want the kernel to check it top-down, we need to
    // copy the previous frames into the end of the string
    memset(tmp, 0, strlen(serialized)+1);
    memcpy(tmp, serialized, strlen(serialized));
  }
  
  memset(out, 0, INT32_STR_SIZE+2+strlen(serialized));
  ret = sprintf(out, "%d,%s", node_count, tmp);
  rlog("[%s] Serialized call stack: %s\n", __func__, out);
 out:
    serialized = NULL;
    ser_len = 1;
    node_count = 0;
    *cs_str = out;
    return ret;
}

static int read_policy_file(const char *policy_fname, char **buf) {
    char *buffer = 0;
    int length;
    int read;
    FILE * f = fopen(policy_fname, "r");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = pyr_alloc_critical_runtime_state(length+1);
        if (!buffer) {
	  printf("[%s] Could not allocate the protected buffer\n", __func__);
            goto fail;
        }
#ifdef MEMDOM_BENCH
        record_internal_malloc(length+1);
#endif
        read = fread(buffer, 1, length, f);
        if (read != length) {
          printf("[%s] Bad read length: %d != %d\n", __func__, read, length);
          goto fail;
        }
	buffer[length] = '\0';
        *buf = buffer;
	fclose(f);
        return length;
    }
    else {
        printf("[%s] Could not open the lib policy file %s\n", __func__, policy_fname);
    }
 fail:
    if (buffer) {
#ifdef MEMDOM_BENCH
        record_internal_free(length+1);
#endif
        pyr_free_critical_state(buffer);
    }
    if (f)
        fclose(f);
    *buf = NULL;
    return -1;
}

/* Reads the library policy at in the poliy file at policy_fname,
 * and serializes it for registration with the LSM.
 */
int pyr_parse_lib_policy(const char *policy_fname, char **parsed) {

    int rule_len;
    char *ser = NULL, *out = NULL, *tmp_ser = NULL;
    uint32_t count = 0, ser_len = 1; // for null-byte
    int ret;

    char *policy;
    size_t policy_len = 0;
    char *policyp;
    ret = read_policy_file(policy_fname, &policy);
    if (ret < 0) {
        goto fail;
    }
    policy_len = ret;
    policyp = policy;
    
    // loop through the policy to serialize it into
    // a format that can be interpreted by the LSM
    char *next_rule = strsep(&policy, "\n");
    while(next_rule) {
        if (!strlen(next_rule)) {
	  // this means we've hit an empty line, so skip to next rule
	  next_rule = strsep(&policy, "\n");
	  continue;
	}
      
	rule_len = strlen(next_rule);
	tmp_ser = realloc(ser, ser_len+rule_len);
#ifdef MEMDOM_BENCH
        record_internal_malloc(ser_len+rule_len);
#endif
	if (!tmp_ser) {
	  ret = -1;
	  goto fail;
	}
	// UGH need to clear the very first allocation,
	// so we don't accidentally start concatenating to
	// junk that realloc spits out
	if (ser_len == 1) {
	  memset(tmp_ser, 0, ser_len+rule_len);
	}
	ser = tmp_ser;
	
	strncat(ser, next_rule, rule_len);
	ser_len = strlen(ser)+rule_len;
	count++;
	next_rule = strsep(&policy, "\n");
    }
    
    if (count == 0) {
      // this means our file is malformed
      // bc we don't have a single valid rule line
      printf("[%s] Oops, malformed policy file %s. Rules need to be comma-separated\n", __func__, policy_fname);
      ret = -1;
      goto fail;
    }

    // now we need to pre-append the len so the kernel knows how many
    // nodes to expect to de-serialize
    out = pyr_alloc_critical_runtime_state(strlen(ser)+INT32_STR_SIZE+2);
    if (!out) {
        ret = -1;
        goto fail;
    }
#ifdef MEMDOM_BENCH
    record_internal_malloc(strlen(ser)+INT32_STR_SIZE+2);
#endif
    memset(out, 0, strlen(ser)+INT32_STR_SIZE+2);
    ret = sprintf(out, "%d,%s", count, ser);
    goto done;
    
 fail:
    out = NULL;
    
 done:
    if (policyp) {
#ifdef MEMDOM_BENCH
        record_internal_free(policy_len);
#endif
      pyr_free_critical_state(policyp);
    }
    if (ser) {
#ifdef MEMDOM_BENCH
        record_internal_free(strlen(ser)+1);
#endif
        free(ser);
    }
    *parsed = out;
    return ret;
}
