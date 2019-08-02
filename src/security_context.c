/** Implements the Pyronia security context library used for
 * isolating security-critical runtime state and native libraries
 * into memory domains in Pyronia-aware language runtimes.
 *
 *@author Marcela S. Melara
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <memdom_lib.h>
#include <unistd.h>

#include "security_context.h"
#include "util.h"

int pyr_security_context_alloc(struct pyr_security_context **ctxp,
                               int (*collect_callstack_cb)(void),
			       void (*interpreter_lock_acquire_cb)(void),
			       void (*interpreter_lock_release_cb)(void),
			       bool is_child) {
    int err = -1;
    struct pyr_security_context *c = NULL;
    int interp_memdom = -1;
    int i = 0;

    // we want this to be allocated in the interpreter memdom
    if (!is_child) {
      c = malloc(sizeof(struct pyr_security_context));
      if (!c)
	goto fail;

#ifdef MEMDOM_BENCH
      record_internal_malloc(sizeof(struct pyr_security_context));
#endif
      
      pyr_interp_dom_alloc_t *interp_dom_meta = malloc(sizeof(pyr_interp_dom_alloc_t));
      if (!interp_dom_meta)
	goto fail;
#ifdef MEMDOM_BENCH
      record_memdom_metadata_alloc(sizeof(pyr_interp_dom_alloc_t));
#endif
      if ((interp_memdom = memdom_create()) == -1) {
	printf("[%s] Could not create interpreter dom # %d\n", __func__, 1);
	goto fail;
      }
#ifdef MEMDOM_BENCH
      record_new_domain_page();
#endif

      rlog("[%s] New interpreter memdom: %d\n", __func__, interp_memdom);
      
      // don't forget to add the main thread to this memdom
      smv_join_domain(interp_memdom, MAIN_THREAD);
      if (smv_is_in_domain(interp_memdom, MAIN_THREAD) != 1) {
	printf("[%s] main thread %d not in first interp memdom!!!\n", __func__, smvthread_get_id());
	goto fail;
      }
      
      memdom_priv_add(interp_memdom, MAIN_THREAD, MEMDOM_READ | MEMDOM_WRITE);

      interp_dom_meta->memdom_id = interp_memdom;
      interp_dom_meta->start = memdom_mmap(interp_dom_meta->memdom_id, 0, MEMDOM_HEAP_SIZE,
					   PROT_READ | PROT_WRITE,
					   MAP_PRIVATE | MAP_ANONYMOUS | MAP_MEMDOM, 0, 0);
      if (interp_dom_meta->start == MAP_FAILED) {
	printf("[%s] could not map memdom area\n", __func__);
        goto fail;
      }
      interp_dom_meta->end = interp_dom_meta->start + MEMDOM_HEAP_SIZE;
      interp_dom_meta->has_space = true;
      interp_dom_meta->writable = true;
      interp_dom_meta->nested_grants = 1;
      c->interp_doms = insert_memdom_metadata(interp_dom_meta, NULL);
      if (!c->interp_doms) {
	printf("[%s] could not insert first metadata\n", __func__);
        goto fail;
      }

      c->main_path = NULL;
      // this ensures that we really do revoke write access at the end of pyr_init
      c->nested_grants = 1;
      c->verified_resources = NULL;
      /*if (!collect_callstack_cb) {
        printf("[%s] Need non-null callstack collect callback\n", __func__);
        err = -EINVAL;
        goto fail;
	}*/
      c->collect_callstack_cb = collect_callstack_cb;
      c->interpreter_lock_acquire_cb = interpreter_lock_acquire_cb;
      c->interpreter_lock_release_cb = interpreter_lock_release_cb;
      
      // this list will be added to whenever a new non-builtin extenion
      // is loaded via dlopen
      c->native_libs = NULL;
      
      *ctxp = c;
    }
    return 0;
 fail:
    if (c) {
#ifdef MEMDOM_BENCH
        record_internal_free(sizeof(struct pyr_security_context));
#endif
      free(c);
    }
    *ctxp = NULL;
    return err;
}

int pyr_find_native_lib_memdom(pyr_native_ctx_t *start, const char *lib) {
    pyr_native_ctx_t *runner = start;

    while (runner != NULL) {
        if (!strncmp(runner->library_name, lib, strlen(lib))) {
            printf("[%s] Found memdom %d\n", __func__, runner->memdom_id);
            return runner->memdom_id;
        }
    }
    return -1;
}

void free_interp_dom_metadata(pyr_interp_dom_alloc_t **dom) {
    int memdom_id = -1;
    pyr_interp_dom_alloc_t *d = *dom;
    int i = 0;

    if (!d)
        return;

    rlog("[%s] memdom %d\n", __func__, d->memdom_id);
#ifdef PYR_MEMDOM_BENCH
    printf("%lu\n", memdom_get_peak_metadata_alloc(d->memdom_id));
#endif
    if (d->start) {
        memdom_priv_add(d->memdom_id, MAIN_THREAD, MEMDOM_WRITE);
	memdom_kill(d->memdom_id);
    }
#ifdef MEMDOM_BENCH
      record_memdom_metadata_free(sizeof(pyr_interp_dom_alloc_t));
#endif
    free(d);
    *dom = NULL;
}

void pyr_security_context_free(struct pyr_security_context **ctxp) {
    struct pyr_security_context *c = *ctxp;
    int i = 0;

    if (!c)
        return;

    rlog("[%s] Freeing security context %p\n", __func__, c);

    //pyr_native_lib_context_free(&c->native_libs);
    free_avl_tree(&c->interp_doms);
    c->verified_resources = NULL;
#ifdef MEMDOM_BENCH
    record_internal_free(sizeof(struct pyr_security_context));
#endif
    free(c);
    rlog("[%s] Called from PID %d: %p\n", __func__, getpid(), c);
    *ctxp = NULL;
}
