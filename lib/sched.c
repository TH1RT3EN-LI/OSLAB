#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *  Search through 'envs' for a runnable environment ,
 *  in circular fashion statrting after the previously running env,
 *  and switch to the first such environment found.
 *
 * Hints:
 *  The variable which is for remain_time_slicesing should be defined as 'static'.
 */

extern struct Env_list env_sched_list[];
extern struct Env *curenv;

void sched_yield(void)
{
	static int remain_time_slices = 0; // remaining time slices of current env
	static int shed_level = 0; // current env_sched_list index, 0 or 1

	struct Env *e = curenv;

	// If current env still has time slices and is runnable, continue running it
	if (remain_time_slices > 0 && e && e->env_status == ENV_RUNNABLE) {
		remain_time_slices--;
		env_run(e);
		return;
	}

	// Move current env to the tail if it's not runnable
	if (e) {
		LIST_REMOVE(e, env_sched_link);
		LIST_INSERT_TAIL(&env_sched_list[1 - shed_level], e, env_sched_link);
	}

	// Try to find a runnable env in current list
	while (1) {
		if (LIST_EMPTY(&env_sched_list[shed_level])) {
			shed_level = 1 - shed_level;
			if (LIST_EMPTY(&env_sched_list[shed_level])) { // if both lists are empty, panic, this should not happen
				// panic("empty empty empty env_sched_list");
				return ; // or return, no runnable env found
			}
		}

		e = LIST_FIRST(&env_sched_list[shed_level]);
		if (!e) continue; // if no env found, continue to next iteration

		if (e->env_status == ENV_RUNNABLE) {
			break;
		} else if (e->env_status == ENV_NOT_RUNNABLE) {
			LIST_REMOVE(e, env_sched_link);
			LIST_INSERT_TAIL(&env_sched_list[1 - shed_level], e, env_sched_link);
		} else if (e->env_status == ENV_FREE) {
			LIST_REMOVE(e, env_sched_link);
		}
	}

	remain_time_slices = e->env_pri > 0 ? e->env_pri - 1 : 0;
	env_run(e);
}