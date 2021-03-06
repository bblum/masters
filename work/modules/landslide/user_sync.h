/**
 * @file user_sync.h
 * @brief state for modeling userspace synchronization behaviour
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __LS_USER_SYNC_H
#define __LS_USER_SYNC_H

#include "variable_queue.h"

struct agent;
struct hax;
struct ls_state;

/* a dynamically-allocated part of a mutex. */
struct mutex_chunk {
	Q_NEW_LINK(struct mutex_chunk) nobe;
	unsigned int base;
	unsigned int size;
};

Q_NEW_HEAD(struct mutex_chunks, struct mutex_chunk);

/* a single mutex */
struct mutex {
	Q_NEW_LINK(struct mutex) nobe;
	unsigned int addr;
	struct mutex_chunks chunks;
};

Q_NEW_HEAD(struct mutexes, struct mutex);

/* the state of all mutexes known in userspace. */
struct user_sync_state {
	unsigned int mutex_size;
	/* list of all known mutexes in userspace. note that mutexes are only
	 * placed on this list if mutex_init is observed to malloc. */
	struct mutexes mutexes;
	/* state machine for the currently-executing thread to guess whether it's
	 * stuck in a userspace yield loop. at the start of each transition. reset
	 * at the beginning of each transition; if it says "yielded but didn't do
	 * anything else interesting" at the next decision point, the current
	 * thread's yield loop counter will be incremented at the checkpoint. */
	enum { NOTHING_INTERESTING, YIELDED, ACTIVITY } yield_progress;
	/* extra bonus part of the state machine for detecting tight spin-loops
	 * around xchg (or cmpxchg). */
	unsigned int xchg_count;
	bool xchg_loop_has_pps;
};

struct user_yield_state {
	/* how many transitions have gone by where the user did "nothing but"
	 * spin in a yield loop? in oldscheds in the tree, this can have values
	 * 0 to TOO_MANY_YIELDS. */
	unsigned int loop_count;
	/* flag associated with the above; used at the end of each branch to
	 * retroactively mark as blocked the transitions preceding the one where
	 * its yield counter hit the maximum. */
	bool blocked;
};

/* how many calls to yield makes us consider a user thread to be blocked? */
#define TOO_MANY_YIELDS 10

/* how many times spinning around xchg/cmpxchg means a thread is blocked? */
#define TOO_MANY_XCHGS_TIGHT_LOOP 100
/* in case of a DR PP on the xchg, avoid O(n^2) PP comparisons with huge n */
#define TOO_MANY_XCHGS_WITH_PPS 20
#define TOO_MANY_XCHGS(u) ((u)->xchg_loop_has_pps ? TOO_MANY_XCHGS_WITH_PPS \
                                                  : TOO_MANY_XCHGS_TIGHT_LOOP)
#define XCHG_BLOCKED(y) \
	({ const struct user_yield_state *____y = (y); /* don't shadow __y */ \
	   ____y->loop_count == TOO_MANY_XCHGS_TIGHT_LOOP || \
	   ____y->loop_count == TOO_MANY_XCHGS_WITH_PPS; })

#define agent_is_user_yield_blocked(y) \
	 ({ const struct user_yield_state *__y = (y); \
	    assert(__y->loop_count <= TOO_MANY_YIELDS || XCHG_BLOCKED(__y)); \
	    __y->loop_count == TOO_MANY_YIELDS || \
	    XCHG_BLOCKED(__y) || __y->blocked; })

#define agent_has_yielded(y) \
	 ({ const struct user_yield_state *__y = (y); \
	    assert(__y->loop_count <= TOO_MANY_YIELDS || XCHG_BLOCKED(__y)); \
	    __y->loop_count > 0 || __y->blocked; })

#define agent_has_xchged(u) \
	({ assert((u)->xchg_count <= TOO_MANY_XCHGS_TIGHT_LOOP); \
	   STATIC_ASSERT(TOO_MANY_XCHGS_TIGHT_LOOP > TOO_MANY_XCHGS_WITH_PPS); \
	   (u)->xchg_count > 0; })

void user_sync_init(struct user_sync_state *u);
void user_yield_state_init(struct user_yield_state *y);

/* user mutexes interface  */

void learn_malloced_mutex_structure(struct user_sync_state *u, unsigned int lock_addr,
									unsigned int chunk_addr, unsigned int chunk_size);
void mutex_destroy(struct user_sync_state *u, unsigned int lock_addr);
void check_user_mutex_access(struct ls_state *ls, unsigned int addr);

/* user yield-loop-blocking interface  */

/* dpor-related */
void update_user_yield_blocked_transitions(struct hax *h);
bool is_user_yield_blocked(struct hax *h);
/* scheduler-related */
void check_user_yield_activity(struct user_sync_state *u, struct agent *a);
void check_user_xchg(struct user_sync_state *u, struct agent *a);
void record_user_yield_activity(struct user_sync_state *u);
void record_user_mutex_activity(struct user_sync_state *u);
void record_user_xchg_activity(struct user_sync_state *u);
void record_user_yield(struct user_sync_state *u);
/* memory-related */
void check_unblock_yield_loop(struct ls_state *ls, unsigned int addr);

#endif
