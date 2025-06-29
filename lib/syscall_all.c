#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char)c);
	return;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0)
	{
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
void sys_yield(void)
{
	// save a photo of the current kernel stack to TIMESTACK
	bcopy(
		(void *)KERNEL_SP - sizeof(struct Trapframe), 
		(void *)TIMESTACK - sizeof(struct Trapframe),
		sizeof(struct Trapframe)
	);
	// sched the next process in the list to run
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
	{
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;

	ret = envid2env(envid, &env, 1);
	if (ret < 0)
	{
		return ret;
	}

	env->env_pgfault_handler = func;
	env->env_xstacktop = xstacktop;

	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// sysno is system call number, maped with different syscall procedure
	// but it is not used in this function
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret; // in earlier lab r for return , now ret for return value...

	ret = 0;
	if (va >= UTOP) // only allow mapping below UTOP
	{
		return -E_INVAL;
	}
	if (perm & PTE_COW) // PTE_COW means copy on write, which is not allowed here

	{
		return -E_INVAL;
	}
	ret = envid2env(envid, &env, 1); // get process by id
	if (ret < 0)					 // does't exist or permission denied
	{
		return ret;
	}
	ret = page_alloc(&ppage); // give a page
	if (ret < 0)
	{
		return ret;
	}
	ret = page_insert(env->env_pgdir, ppage, va, perm); // map the page to the process
	if (ret < 0)
	{
		return ret;
	}
	return 0;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG); // make sure the address is page aligned
	round_dstva = ROUNDDOWN(dstva, BY2PG);

	if (srcva >= UTOP || dstva >= UTOP) // space safe garanteed
	{
		return -E_INVAL;
	}

	ret = envid2env(srcid, &srcenv, 1);
	if (ret < 0)
	{
		return ret;
	}
	ret = envid2env(dstid, &dstenv, 1);
	if (ret < 0)
	{
		return ret;
	}

	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);
	if (ppage == NULL)
	{
		return -E_INVAL;
	}
	if (((*ppte & PTE_R) == 0) && ((perm & PTE_R) == 1)) // if the page is not readable, bad
	{
		return -E_INVAL;
	}
	if (perm & PTE_COW) // same as sys_mem_alloc, PTE_COW is not allowed here
	{
		return -E_INVAL;
	}
	ppage = pa2page(PTE_ADDR(*ppte)); // get the page from the page table entry
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	if (ret < 0)
	{
		return ret;
	}

	ret = 0;
	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// printf("unmap %x %x\n",envid, va);
	//  Your code here.
	int ret;
	struct Env *env;

	if (va >= UTOP)
	{
		return -E_INVAL;
	}

	ret = envid2env(envid, &env, 1);
	if (ret < 0)
	{
		return ret;
	}

	page_remove(env->env_pgdir, va);

	return ret;
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
int sys_env_alloc(void)
{
	// allocate a new environment
	struct Env *e;
	int r = env_alloc(&e, curenv->env_id);
	if (r < 0)
		return r;

	// set initial status and priority
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri; // why？

	// save the current environment's trapframe
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), (void *)&e->env_tf, sizeof(struct Trapframe));

	// set up the trapframe for the child.
	e->env_tf.pc = e->env_tf.cp0_epc;
	e->env_tf.regs[2] = 0; // child, return value is 0 
	// regs[2] is v0, should be the return value of syscall

	return e->env_id;
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;
	extern struct Env_list env_sched_list[];
	struct Env *o;
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE)
	{
		return -E_INVAL;
	}
	ret = envid2env(envid, &env, 1);
	if (ret < 0)
	{
		return ret;
	}
	// printf("set status %x %d %x\n",envid, env->env_status, status);
	/*
	LIST_FOREACH(o, &env_sched_list[0], env_sched_link)
	{
		printf("%x ",o->env_id);
	}
	printf("\n");
	LIST_FOREACH(o, &env_sched_list[1], env_sched_link)
	{
		printf("%x ",o->env_id);
	}
	printf("\n");
	*/
	if (status == ENV_FREE)
	{
		env_destroy(env);
	}
	else
	{
		env->env_status = status;
	}
	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
void sys_ipc_recv(int sysno, u_int dstva)
{
	if (dstva >= UTOP) {
		return;
	}

	struct Env *env = curenv;
	env->env_ipc_recving = 1;
	env->env_ipc_dstva = dstva;
	env->env_status = ENV_NOT_RUNNABLE;

	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva, u_int perm)
{
	if ((srcva >= UTOP && srcva != 0))
		return -E_INVAL;

	struct Env *e;
	int r = envid2env(envid, &e, 0);
	if (r < 0)
		return r;

	if (!e->env_ipc_recving)
		return -E_IPC_NOT_RECV;

	e->env_ipc_value = value;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_perm = 0;

	if (srcva != 0 && e->env_ipc_dstva != 0) {
		struct Page *p;
		Pte *pte;
		p = page_lookup(curenv->env_pgdir, ROUNDDOWN(srcva, BY2PG), &pte);
		if (!p)
			return -E_INVAL;
		r = page_insert(e->env_pgdir, p, ROUNDDOWN(e->env_ipc_dstva, BY2PG), perm);
		if (r < 0)
			return r;
		e->env_ipc_perm = perm;
	}

	e->env_ipc_recving = 0;
	e->env_status = ENV_RUNNABLE;
	return 0;
}
