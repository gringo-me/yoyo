#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/pid.h>
#include <linux/cpumask.h>
#include <asm-generic/barrier.h>
#include <linux/futex.h>
#include <asm/futex.h>
#include <linux/log2.h>
#include "../kernel/sched/sched.h"

static struct rq *task_rq_lock(struct task_struct *p,unsigned long *flags);
static inline void task_rq_unlock(struct rq*, struct task_struct *p, unsigned long *flags);

void dequeue_task(struct rq *rq, struct task_struct *p, int flags);

void enqueue_task(struct rq *rq, struct task_struct *p, int flags);

/*
 * The task state array is a strange "bitmap" of
 * reasons to sleep. Thus "running" is zero, and
 * you can test for combinations of others with
 * simple bit tests.
 */
static const char * const task_state_array[] = {
        "R (running)",          /*   0 */
        "S (sleeping)",         /*   1 */
        "D (disk sleep)",       /*   2 */
        "T (stopped)",          /*   4 */
        "t (tracing stop)",     /*   8 */
        "Z (zombie)",           /*  16 */
        "X (dead)",             /*  32 */
        "x (dead)",             /*  64 */
        "K (wakekill)",         /* 128 */
        "W (waking)",           /* 256 */
        "P (parked)",           /* 512 */
};

static inline const char *get_task_state(struct task_struct *tsk)
{
        unsigned int state = (tsk->state & TASK_REPORT) | tsk->exit_state;
        const char * const *p = &task_state_array[0];

        BUILD_BUG_ON(1 + ilog2(TASK_STATE_MAX) != ARRAY_SIZE(task_state_array));

        while (state) {
                p++;
                state >>= 1;
        }
        return *p;
}

/* system call to set the new field in 
 * task struct 'futex' that allows 
 * a task to know which locks it helds 
 * in the kernelspace.
 */

asmlinkage long sys_set_task_futex(pid_t pid, u32 *uaddr)
{
	struct rq *rq;
	struct pid *pid_struct;
	struct task_struct *p, *waiter = NULL;
	unsigned long flags; 
	int on_rq, running, oldprio,waitpid=0;

	pid_struct = find_get_pid(pid);
	p = pid_task(pid_struct,PIDTYPE_PID);
	rq = task_rq_lock(p,&flags);
	oldprio = p->prio;
	on_rq = p->on_rq;
	running = task_current(rq, p);

	if (on_rq)
		dequeue_task(rq, p, 0);
	if (running)
		p->sched_class->put_prev_task(rq, p);

	p->futex = uaddr;

	if (running)
		p->sched_class->set_curr_task(rq);
	if (on_rq)
		enqueue_task(rq, p, ENQUEUE_HEAD );

	task_rq_unlock(rq,p,&flags);
	if(uaddr) waiter = get_top_waiter_diff_cpu(uaddr,1);
	if(waiter) {
		//p->waiter = waiter;
		waitpid = (int)waiter->pid;
		printk(KERN_EMERG "cpu %d Id %d has futex: %d | %d is %s\n", rq->cpu,p->pid,p->futex,waitpid,get_task_state(waiter)); 
	};
	return 0;
}

