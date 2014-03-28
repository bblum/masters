/**
 * @file memory.h
 * @brief routines for tracking dynamic allocations and otherwise shared memory
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __LS_MEMORY_H
#define __LS_MEMORY_H

#include <simics/api.h> /* for bool, of all things... */

#include "lockset.h"
#include "rbtree.h"

struct hax;

/******************************************************************************
 * Shared memory access tracking
 ******************************************************************************/

struct mem_lockset {
	int eip;
	struct lockset locks_held;
	Q_NEW_LINK(struct mem_lockset) nobe;
};

Q_NEW_HEAD(struct mem_locksets, struct mem_lockset);

/* represents an access to shared memory */
struct mem_access {
	int addr;      /* byte granularity */
	bool write;    /* false == read; true == write */
	/* PC is recorded per-lockset, so when there's a data race, the correct
	 * eip can be reported instead of the first one. */
	//int eip;       /* what instruction pointer */
	int other_tid; /* does this access another thread's stack? 0 if none */
	int count;     /* how many times accessed? (stats) */
	bool conflict; /* does this conflict with another transition? (stats) */
	struct mem_locksets locksets; /* distinct locksets used while accessing */
	struct rb_node nobe;
};

/******************************************************************************
 * Heap state tracking
 ******************************************************************************/

/* a heap-allocated block. */
struct chunk {
	int base;
	int len;
	struct rb_node nobe;
	/* for use-after-free reporting */
	char *malloc_trace;
	char *free_trace;
};

struct mem_state {
	struct rb_root heap;
	int heap_size;
	/* dynamic allocation request state */
	bool guest_init_done;
	bool in_mm_init; /* userspace only */
	bool in_alloc;
	bool in_free;
	int alloc_request_size; /* valid iff in_alloc */
	int cr3; /* 0 == uninitialized or this is for kernel mem */
	int cr3_tid; /* tid for which cr3 was registered (main tid of process) */
	int user_mutex_size; /* 0 == uninitialized or kernel mem as above */
	/* set of all shared accesses that happened during this transition;
	 * cleared after each save point - done in save.c */
	struct rb_root shm;
	/* set of all chunks that were freed during this transition; cleared
	 * after each save point just like the shared memory one above */
	struct rb_root freed;
};

/******************************************************************************
 * Interface
 ******************************************************************************/

void mem_init(struct ls_state *);

void mem_update(struct ls_state *);

void mem_check_shared_access(struct ls_state *, int phys_addr, int virt_addr,
							 bool write);
bool mem_shm_intersect(conf_object_t *cpu, struct hax *h0, struct hax *h2,
                       bool in_kernel);

bool shm_contains_addr(struct mem_state *m, int addr);

#endif
