/* Main Pyronia userspace API.
*
*@author Marcela S. Melara
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <smv_lib.h>
#include <memdom_lib.h>
#include <openssl/sha.h>

#include "pyronia_lib.h"
#include "security_context.h"
#include "serialization.h"
#include "si_comm.h"
#include "util.h"

#ifdef PYRONIA_BENCH
#include "benchmarking_util.h"
#endif

#ifdef WITH_STACK_LOGGING
#include "policy_avl_tree.h"
#include "stack_log.h"
// don't collect stack for logging if true since the interp is not fully initialized yet
static int in_init = 1; 
#endif

static struct pyr_security_context *runtime = NULL;
static int pyr_smv_id = -1;
static int si_smv_id = -1;
static int is_build = 0;
static pthread_t recv_th;
static int interp_memdom_pool_size = 1;

static inline int init_child_memdoms() {
  int i = 0;
  int err = -1;
  
  // use one extra for the SI thread's memdom
  for (i = 0; i <= interp_memdom_pool_size; i++) {
    err = memdom_register_new();
    if (err == -1) {
      printf("[%s] Could not register new memdom %d\n", __func__, i);
      goto out;
    }
    else {
      rlog("[%s] Registered new memdom\n", __func__);
      if (si_memdom != -1 && err == si_memdom)
	continue;
      smv_join_domain(err, MAIN_THREAD);
      memdom_priv_add(err, MAIN_THREAD, MEMDOM_READ);
      smv_join_domain(err, si_smv_id);
      memdom_priv_add(err, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);
    }
  }
  err = 0;
 out:
  return err;
}

static void avl_set_space(avl_node_t *n) {
    pyr_interp_dom_alloc_t *dalloc = NULL;
    if (n == NULL || n->memdom_metadata == NULL)
        return;

    n->memdom_metadata->has_space = true;
    avl_set_space(n->left);
    avl_set_space(n->right);
}

/** Do all the necessary setup for a language runtime to use
 * the Pyronia extensions: open the stack inspection communication
 * channel and initialize the SMV backend.
 * Note: This function revokes access to the interpreter domain at the end.
 */
int pyr_init(const char *main_mod_path,
             const char *lib_policy_file,
             int (*collect_callstack_cb)(void),
             void (*interpreter_lock_acquire_cb)(void),
             void (*interpreter_lock_release_cb)(void),
	     int is_child) {
    int err = 0, i = 0;
    char *policy = NULL;
    pthread_mutexattr_t attr;

#ifdef WITH_STACK_LOGGING
    in_init = 1;
#endif
    rlog("[%s] Initializing pyronia for module %s in proc %d\n", __func__, main_mod_path, getpid());
    
    // We make an exception for setup.py and the sysconfig modules
    // so we don't somehow clobber installs with Pyronia checks
    if (main_mod_path == NULL || !strcmp(main_mod_path, "../setup.py") ||
        !strcmp(main_mod_path, "sysconfig")) {
      is_build = 1;
      runtime = NULL;
      return 0;
    }

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&security_ctx_mutex, &attr);
    pthread_cond_init(&si_cond_var, NULL);

    /* Register with the memdom subsystem */
    // We don't want the main thread's memdom to be
    // globally accessible, so init with 0.
    err = smv_main_init(0, is_child);
    if (err < 0) {
      printf("[%s] Memdom subsystem registration failure\n", __func__);
      goto out;
    }

    /* If this is being called from a child proc, need to re-create
       all memdoms in kernel. Nothing else since userspace and kernel-level
       mappings still exist */
    if (is_child) {
      err = smv_create();
      if (err != si_smv_id) {
	printf("[%s] SI smv re-registration for child proc failed\n", __func__);
	goto out;
      }
      smv_join_domain(MAIN_THREAD, si_smv_id);
      memdom_priv_add(MAIN_THREAD, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);
      err = init_child_memdoms();
      if (err) {
	printf("[%s] Memdom re-registration for child proc failed\n", __func__);
	goto out;
      }
      avl_set_space(runtime->interp_doms);
    }
    
    /* Initialize the runtime's security context */
    err = pyr_security_context_alloc(&runtime, collect_callstack_cb,
                                     interpreter_lock_acquire_cb,
                                     interpreter_lock_release_cb, is_child);
    if (!err) {
        err = set_str(main_mod_path, &runtime->main_path);
    }
    if (err) {
        printf("[%s] Runtime initialization failure\n", __func__);
        goto out;
    }
    runtime->nested_grants = 1; // this ensures the child starts from scatch

    if (!is_child) {
      /* Parse the library policy from disk */
      err = pyr_parse_lib_policy(lib_policy_file, &policy);
      if (err < 0) {
	printf("[%s] Parsing lib policy failure: %d\n", __func__, err);
	goto out;
      }
    }
    is_inspecting_stack = true;
    /* Initialize the stack inspection communication channel with
     * the kernel */
    if (!is_child) {
      err = pyr_init_si_comm(policy, is_child);
      if (err) {
	printf("[%s] SI comm channel initialization failed\n", __func__);
	goto out;
      }
#ifdef WITH_STACK_LOGGING
      in_init = 0;
#endif
      
      //PyEval_InitThreads(); // needed to enable stack inspector

      err = pyr_callstack_req_listen(is_child);
      if (err)
	goto out;
    }
    else {
      is_inspecting_stack = false;
    }
    
    // create another SMV to be used by threads originally created by
    // pthread_create. We won't allow mixing pthreads woth smvthreads
    /*pyr_smv_id = smv_create();
    if (pyr_smv_id == -1) {
      printf("[%s] Could not create an SMV for pyronia threads\n", __func__);
      goto out;
    }
    // We need this SMV to be able to access any Python functions
    smv_join_domain(MAIN_THREAD, pyr_smv_id);
    memdom_priv_add(MAIN_THREAD, pyr_smv_id, MEMDOM_READ | MEMDOM_WRITE);
    */
    
    pyr_is_inspecting(); // we want to wait for the listener to be ready
 out:
    if (policy)
      pyr_free_critical_state(policy);
    /* Revoke access to the interpreter domain now */
    pyr_revoke_critical_state_write(NULL);
    if (!err) {
      rlog("[%s] Initialized pyronia extensions\n", __func__);
    }
    else {
      pyr_exit();
    }
#ifdef WITH_STACK_LOGGING
    in_init = 0;
#endif
    return err;
}

static pyr_interp_dom_alloc_t *new_interp_memdom() {
  int interp_memdom = -1;
  pyr_interp_dom_alloc_t *new_dom = NULL;

#ifdef PYRONIA_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif

  if (interp_memdom_pool_size+1 > MAX_NUM_INTERP_DOMS)
      goto fail;

  new_dom = malloc(sizeof(pyr_interp_dom_alloc_t));
  if (!new_dom)
      goto fail;

  interp_memdom = memdom_create();
  if(interp_memdom == 0 || interp_memdom == -1) {
    printf("[%s] Bad new memdom id %d with %d pool size\n", __func__, interp_memdom, interp_memdom_pool_size);
    goto fail;
  }
  // don't forget to add the main thread to this memdom
  smv_join_domain(interp_memdom, MAIN_THREAD);
  memdom_priv_add(interp_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);

  new_dom->memdom_id = interp_memdom;
  new_dom->start = memdom_mmap(new_dom->memdom_id, 0, MEMDOM_HEAP_SIZE,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_MEMDOM, 0, 0);
  if (new_dom->start == NULL) {
      goto fail;
  }
  new_dom->end = new_dom->start + MEMDOM_HEAP_SIZE;
  new_dom->has_space = true;
  new_dom->writable = true;

  // for big profiles, we're going to be calling this functions before
  // we even launch the SI thread
  if (si_smv_id != -1) {
    smv_join_domain(new_dom->memdom_id, si_smv_id);
    memdom_priv_add(new_dom->memdom_id, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);
  }

  interp_memdom_pool_size++;
  rlog("[%s] new memdom %d\n", __func__, new_dom->memdom_id);
  goto out;
 fail:
  free_interp_dom_metadata(&new_dom);
  new_dom = NULL;
 out:
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_memdom_creat(start, stop);
#endif
  return new_dom;
}

void print_avl(avl_node_t *n) {
  if (n != NULL) {
    printf("%d \n", n->memdom_metadata->memdom_id);
    print_avl(n->left);
    print_avl(n->right);
  }
}

/** Wrapper around memdom_alloc in the interpreter domain.
 */
void *pyr_alloc_critical_runtime_state(size_t size) {
    void *new_block = NULL;
    int i = 0;
    pyr_interp_dom_alloc_t *dalloc = NULL;

#ifdef PYRONIA_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif

    if (is_build) {
        new_block =  malloc(size);
        goto out;
    }

    if (!runtime)
        goto out;

    if(size > MEMDOM_HEAP_SIZE) {
      rlog("[%s] Requested size is too large for interpreter dom.\n", __func__);
      new_block = (void *)1;
      goto out;
    }

    pthread_mutex_lock(&security_ctx_mutex);
    dalloc = find_memdom_with_space(runtime->interp_doms, size);
    if (dalloc) {
        new_block = memdom_alloc(dalloc->memdom_id, size);

        if (new_block) {
	  rlog("[%s] Allocated %lu bytes in memdom %d\n", __func__, size, dalloc->memdom_id);
	  goto out;
        }
        else {
            dalloc->has_space = false;
            rlog("[%s] Memdom allocator could not find a suitable block in memdom %d. Current number of active memdoms: %d\n", __func__, dalloc->memdom_id, interp_memdom_pool_size);
        }
    }

    // if we get here, no memdoms have space for the current allocation
    rlog("[%s] Not enough space in any active memdoms. Current number of active memdoms: %d\n", __func__, interp_memdom_pool_size);
    dalloc = new_interp_memdom();
    if (!dalloc)
        goto out;
    new_block = memdom_alloc(dalloc->memdom_id, size);
    if (!new_block) {
        rlog("[%s] Error: could not allocate memory in new memdom\n", __func__);
        goto out;
    }
    runtime->interp_doms = insert_memdom_metadata(dalloc, runtime->interp_doms);
    //print_avl(runtime->interp_doms);
 out:
    pthread_mutex_unlock(&security_ctx_mutex);
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_malloc(start, stop);
#endif
    return new_block;
}

static pyr_interp_dom_alloc_t *get_interp_dom_memdom(void *op) {
    int id = 0;
    int i = 0;
    pyr_interp_dom_alloc_t *dalloc = NULL;

    pthread_mutex_lock(&security_ctx_mutex);
    dalloc = find_memdom_metadata(op, runtime->interp_doms);
    pthread_mutex_unlock(&security_ctx_mutex);
    return dalloc;
}

/** Wrapper around memdom_free in the interpreter domain.
 * Returns 1 if the state was freed, 0 otherwise.
 */
int pyr_free_critical_state(void *op) {
    pyr_interp_dom_alloc_t *dalloc = NULL;
    bool freed = false;

#ifdef PYRONIA_BENCH
    struct timespec start, stop;
    get_cpu_time(&start);
#endif

    if (is_build) {
        goto out;
    }

    if (!runtime)
        goto out;

    dalloc = get_interp_dom_memdom(op);
    if (dalloc && dalloc->memdom_id > 0) {
        pthread_mutex_lock(&security_ctx_mutex);
        memdom_free_from(dalloc->memdom_id, op);
        if (dalloc->has_space == false) {
            dalloc->has_space = true;
        }
        pthread_mutex_unlock(&security_ctx_mutex);
        rlog("[%s] Freed %p\n", __func__, op);
        freed = true;
    }
 out:
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_free(start, stop);
#endif
    return freed;
}

/** Wrapper around memdom_query_id. Returns 1 if the
 * given pointer is in the interpreter_dom, 0 otherwise.
 */
int pyr_is_critical_state(void *op) {
    pyr_interp_dom_alloc_t *dalloc = NULL;

    if (is_build)
        return 0;

    if (!runtime)
        return 0;

    dalloc = get_interp_dom_memdom(op);
    if (!dalloc)
      return 0;

    return (dalloc->memdom_id > 0);
}

static void avl_set_writable(avl_node_t *n) {
    pyr_interp_dom_alloc_t *dalloc = NULL;
#ifdef PYRONIA_BENCH
    struct timespec start, stop;
#endif

    if (n == NULL || n->memdom_metadata == NULL)
        return;

    dalloc = n->memdom_metadata;
    if (dalloc->has_space) {
#ifdef PYRONIA_BENCH
        get_cpu_time(&start);
#endif
        memdom_priv_add(dalloc->memdom_id, MAIN_THREAD, MEMDOM_WRITE);
#ifdef PYRONIA_BENCH
        get_cpu_time(&stop);
        record_priv_add(start, stop);
#endif
        dalloc->writable = true;
	rlog("[%s] Granted write access to memdom %d\n", __func__, dalloc->memdom_id);
    }
    avl_set_writable(n->left);
    avl_set_writable(n->right);
}

static void avl_set_readonly(avl_node_t *n) {
    pyr_interp_dom_alloc_t *dalloc= NULL;
    if (n == NULL || n->memdom_metadata == NULL)
        return;

#ifdef PYRONIA_BENCH
    struct timespec start, stop;
#endif

    dalloc = n->memdom_metadata;
    if (dalloc->writable) {
#ifdef PYRONIA_BENCH
        get_cpu_time(&start);
#endif
        memdom_priv_del(dalloc->memdom_id, MAIN_THREAD, MEMDOM_WRITE);
#ifdef PYRONIA_BENCH
        get_cpu_time(&stop);
        record_priv_del(start, stop);
#endif
        dalloc->writable = false;
        rlog("[%s] Revoked write access to memdom %d\n", __func__, dalloc->memdom_id);
    }
    avl_set_readonly(n->left);
    avl_set_readonly(n->right);
}

/** Grants the main thread write access to the interpreter domain.
 */
void pyr_grant_critical_state_write(void *op) {
    int i = 0;
    pyr_interp_dom_alloc_t *dalloc = NULL;

    if (is_build)
        return;

    // suspend if the stack tracer thread is running
    pyr_is_inspecting();

#ifdef PYRONIA_BENCH
    struct timespec start, stop, priv_start, priv_stop;
    get_cpu_time(&start);
#endif

    // let's skip adding write privs if our runtime
    // doesn't have a domain or our domain is invalid
    if (!runtime) {
        goto out;
    }

    // if the caller has given us an insecure object, exit
    if (op) {
      dalloc = get_interp_dom_memdom(op);
      rlog("[%s] grant access to obj %p?\n", __func__, op);
      if (!dalloc || dalloc->memdom_id <= 0)
          goto out;
    }

    pthread_mutex_lock(&security_ctx_mutex);
    // if the caller has given us an existing secure object to
    // modify, we should just go ahead an grant that particular
    // memdom the write access
    if (op && !dalloc->writable) {
#ifdef PYRONIA_BENCH
        get_cpu_time(&priv_start);
#endif
      memdom_priv_add(dalloc->memdom_id, MAIN_THREAD, MEMDOM_WRITE);
#ifdef PYRONIA_BENCH
      get_cpu_time(&priv_stop);
      record_priv_add(priv_start, priv_stop);
#endif
      dalloc->writable = true;
      rlog("[%s] Granted write access to obj in memdom %d\n", __func__, dalloc->memdom_id);
      goto granted;
    }

    rlog("[%s] Grants: %d\n", __func__, runtime->nested_grants);

    // slight optimization: if we've already granted access
    // let's avoid another downcall to change the memdom privileges
    // and simply keep track of how many times we've granted access
    if (runtime->nested_grants == 0) {
        avl_set_writable(runtime->interp_doms);
    }
 granted:
    runtime->nested_grants++;
    pthread_mutex_unlock(&security_ctx_mutex);
 out:
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_grant(start, stop);
#endif
    return;
}

/** Revokes the main thread's write privileges to the interpreter domain.
 */
void pyr_revoke_critical_state_write(void *op) {
    int i = 0;
    pyr_interp_dom_alloc_t *dalloc = NULL;
    if (is_build)
      return;

    // suspend if the stack tracer thread is running
    pyr_is_inspecting();

#ifdef PYRONIA_BENCH
    struct timespec start, stop, priv_start, priv_stop;
    get_cpu_time(&start);
#endif

    // let's skip adding write privs if our runtime
    // doesn't have a domain or our domain is invalid
    if (!runtime) {
        goto out;
    }

    // if the caller has given us an insecure object, exit
    if (op) {
      dalloc = get_interp_dom_memdom(op);
      rlog("[%s] revoke access from obj %p?\n", __func__, op);
      if (!dalloc || dalloc->memdom_id <= 0)
        goto out;
    }

    pthread_mutex_lock(&security_ctx_mutex);
    rlog("[%s] Number of grants: %d\n", __func__, runtime->nested_grants);
    runtime->nested_grants--;
    if (op && runtime->nested_grants == 0) {
#ifdef PYRONIA_BENCH
        get_cpu_time(&priv_start);
#endif
      memdom_priv_del(dalloc->memdom_id, MAIN_THREAD, MEMDOM_WRITE);
#ifdef PYRONIA_BENCH
      get_cpu_time(&priv_stop);
      record_priv_del(priv_start, priv_stop);
#endif
      dalloc->writable = false;
      rlog("[%s] Revoked write access for obj in domain %d\n", __func__, dalloc->memdom_id);
      goto revoked;
    }

    // same optimization as above
    if (runtime->nested_grants == 0) {
        avl_set_readonly(runtime->interp_doms);
    }
 revoked:
    pthread_mutex_unlock(&security_ctx_mutex);
 out:
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_revoke(start, stop);
#endif
    return;
}

/** Starts an SMV thread that has access to the MAIN_THREAD memdom.
 * I.e. this is a wrapper for smvthread_create that is supposed to be used
 * in the language runtime as a replacement for all pthread_create calls.
 * This is needed because SMV doesn't allow you to spawn other smvthreads
 * to run in the MAIN_THREAD smv.
 */
int pyr_thread_create(pthread_t* tid, const pthread_attr_t *attr,
                      void*(fn)(void*), void* args) {
    int ret = 0;

#ifdef PYR_INTERCEPT_PTHREAD_CREATE
#undef pthread_create
#endif

    // suspend if the stack tracer thread is running
    pyr_is_inspecting();

    ret = smvthread_create_attr(pyr_smv_id, tid, attr, fn, args);
#ifdef PYR_INTERCEPT_PTHREAD_CREATE
    if (ret > 0)
      ret = 0; // users of pthread_create expect status 0 on success
#define pthread_create(tid, attr, fn, args) pyr_thread_create(tid, attr, fn, args)
#endif

    printf("[%s] Created new Pyronia thread to run in SMV %d\n", __func__, smvthread_get_id());
    return ret;
}

static void avl_join_si(avl_node_t *n, int si_smv_id) {
    pyr_interp_dom_alloc_t *dalloc= NULL;
    if (n == NULL || n->memdom_metadata == NULL)
        return;

    dalloc = n->memdom_metadata;
    if (dalloc->writable) {
        smv_join_domain(dalloc->memdom_id, si_smv_id);
        memdom_priv_add(dalloc->memdom_id, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);
        rlog("[%s] Revoked write access to memdom %d\n", __func__, dalloc->memdom_id);
    }
    avl_set_readonly(n->left);
    avl_set_readonly(n->right);
}

static void *si_memdom_addr = NULL; 
void *get_si_memdom_addr() {
  return si_memdom_addr;
}

struct pyr_security_context *pyr_get_runtime_sec_ctx(void) {
  return runtime;
}

/** Starts the SI listener and dispatch thread.
 */
int pyr_callstack_req_listen(bool is_child) {
    pthread_attr_t attr;
    int i = 0;
    void *tmp = NULL;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (!is_child) {
      si_smv_id = smv_create();
      if (si_smv_id == -1) {
	printf("[%s] Could not create and SMV for the SI thread\n", __func__);
	return -1;
      }
      
      // we trust this thread, but also, we need this thread to be able
      // to access the functions
      smv_join_domain(MAIN_THREAD, si_smv_id);
      memdom_priv_add(MAIN_THREAD, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);
      avl_join_si(runtime->interp_doms, si_smv_id);
      
      si_memdom = -1;
      si_memdom = memdom_create();
      if (si_memdom == -1) {
	printf("[%s] Could not create SI thread memdom\n", __func__);
      }
      // temporarily allow main to write to memdom 2 so we can allocate the page
      smv_join_domain(si_memdom, MAIN_THREAD);
      memdom_priv_add(si_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);
      si_memdom_addr = memdom_mmap(si_memdom, 0, MEMDOM_HEAP_SIZE,
				   PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS | MAP_MEMDOM, 0, 0);
      if (si_memdom_addr == NULL) {
	goto fail;
      }
      memdom_priv_del(si_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);
    }
    smv_join_domain(si_memdom, si_smv_id);
    memdom_priv_add(si_memdom, si_smv_id, MEMDOM_READ | MEMDOM_WRITE);

    rlog("[%s] security context %p\n", __func__, runtime);
    smvthread_create_attr(si_smv_id, &recv_th, &attr, pyr_recv_from_kernel, NULL);
    return 0;
 fail:
    printf("[%s] Error allocating SI memdom\n", __func__);
    return -1;
}

int pyr_is_interpreter_build() {
  return is_build;
}

/* Do all necessary teardown actions. */
void pyr_exit() {
    if (is_build) {
      return;
    }

    // suspend if the stack tracer thread is running
    pyr_is_inspecting();

    rlog("[%s] Exiting Pyronia runtime %d\n", __func__, getpid());
    pthread_cancel(recv_th);
    pyr_teardown_si_comm();
#ifdef PYR_MEMDOM_BENCH
    printf("%d\n", interp_memdom_pool_size);
#endif
    if (runtime) {
      pyr_grant_critical_state_write((void *)runtime->main_path);
      if (runtime->main_path)
	pyr_free_critical_state(runtime->main_path);
    }
#ifdef WITH_STACK_LOGGING
    free_pol_avl_tree(&runtime->verified_resources);
#endif
    pyr_security_context_free(&runtime);
}

/** Wrapper around the runtime callstack collection callback
 * to be called by the si_comm component in handle_callstack_request.
 */
int pyr_collect_runtime_callstack() {
    int err = -1;
#ifdef PYRONIA_BENCH
    struct timespec start, stop;
#endif
    pthread_mutex_lock(&security_ctx_mutex);
    runtime->interpreter_lock_acquire_cb();
#ifdef PYRONIA_BENCH
    get_cpu_time(&start);
#endif
    err = runtime->collect_callstack_cb();
#ifdef PYRONIA_BENCH
    get_cpu_time(&stop);
    record_callstack_gen(start, stop);
#endif
    runtime->interpreter_lock_release_cb();
    rlog("[%s] Done collecting callstack\n", __func__);
    pthread_mutex_unlock(&security_ctx_mutex);
    return err;
}

#ifdef WITH_STACK_LOGGING

/** Records a new verified resource for which the call stack
 * is being logged.
 */
int record_verified_resource(const char *resource) {
    int err = 0;
    pthread_mutex_lock(&security_ctx_mutex);
    runtime->verified_resources = insert_resource(resource, runtime->verified_resources);

    if (runtime->verified_resources == NULL) {
        printf("[%s] Error inserting newly verified resource %s\n", __func__, resource);
        err = -1;
    }
    pthread_mutex_unlock(&security_ctx_mutex);
    return err;
}

/** Checks if the given resource is already in the runtime's list
 * of verified resources, and collects the call stack, if it is found
 */
bool check_verified_resource(const char *resource) {
    bool contains = false;
    pthread_mutex_lock(&security_ctx_mutex);
    if (!runtime || in_init)
      goto out;
    contains = contains_resource(resource, runtime->verified_resources);
    rlog("[%s] Logged resource %s? %s\n", __func__, resource,
	   (contains ? "yes" : "no"));
 out:
    pthread_mutex_unlock(&security_ctx_mutex);
    return contains;
}

/** Collects the runtime's current call stack, and computes the SHA256
 * hash. The resulting callstack_hash
 * can then be passed on to the kernel during the normal syscall
 * interface.
 */
int compute_callstack_hash(unsigned char **callstack_hash) {
    char *tmp = NULL;
    int err = -1;
    size_t cs_str_len = 0;
    unsigned char *hash = NULL;
    char cs_str[512];

    if (!runtime || in_init)
      goto out;
    
    // the condition will be set to false at the top of the
    // recv loop (i.e. after this function returns)
    pthread_mutex_lock(&security_ctx_mutex);
    if (!is_inspecting_stack)
      is_inspecting_stack = true;
    pthread_mutex_unlock(&security_ctx_mutex);

    // Collect and serialize the callstack
    memdom_priv_add(si_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);
    err = pyr_collect_runtime_callstack();
    if (err)
        goto out;
    err = finalize_callstack_str(&tmp);
    if (err <= 0) {
        err = -1;
        goto out;
    }
    // copy the string over before we revoke access to the SI dom
    memset(cs_str, 0, strlen(tmp)+1);
    strcpy(cs_str, tmp);
    memset(tmp, 0, MEMDOM_HEAP_SIZE); // don't forget to clear this page
    memdom_priv_del(si_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);
    rlog("[%s] Sending callstack %s (%d bytes) to kernel\n", __func__, cs_str, strlen(cs_str));
    
    // compute the SHA 256 hash of the callstack string;
    // do it after copying it into this space, so OPENSSL doesn't acces
    // the SI memdom
    hash = malloc(SHA256_DIGEST_LENGTH);
    if (!hash)
      goto out;

    hash = SHA256((const unsigned char *)cs_str, strlen(cs_str), hash);
    err = 0;
 out:
    *callstack_hash = hash;
    memset(cs_str, 0, 512);
    pthread_mutex_lock(&security_ctx_mutex);
    is_inspecting_stack = false;
    pthread_cond_broadcast(&si_cond_var);
    pthread_mutex_unlock(&security_ctx_mutex);
    return err;
}

#endif
