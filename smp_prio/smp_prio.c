#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/pid.h>
#include <linux/cpumask.h>
#include <asm-generic/barrier.h>

#include "../kernel/sched/sched.h"

static struct rq *task_rq_lock(struct task_struct *p,unsigned long *flags);
static inline void task_rq_unlock(struct rq*, struct task_struct *p, unsigned long *flags);

void dequeue_task(struct rq *rq, struct task_struct *p, int flags);

void enqueue_task(struct rq *rq, struct task_struct *p, int flags);
inline void check_class_changed(struct rq *rq, struct task_struct *p,
				       const struct sched_class *prev_class,
				       int oldprio);
 void __setscheduler(struct rq *rq, struct task_struct *p, int policy, int prio);

 
 /*
 * set_task_smp_prio_set - Hack to set the priorities per processor
 */
long set_task_smp_prio(const char * prio_per_proc, struct task_struct *p, int size_array)
{
	if(size_array != num_online_cpus())
		return -1;

	p->prio_per_cpu = 1;
	p->smp_prio = (char*) prio_per_proc ;

	printk(KERN_EMERG "SMP Priorities correctly set"); 
	return 0;
}


/* system call to set the new field in 
 * task struct 'smp_prio' that allows 
 * one priority per processor on SMP machines
 */

asmlinkage long sys_set_smp_prio(pid_t pid, const char *smp_prio)
{
	struct rq *rq;
	struct pid *pid_struct;
	struct task_struct *p;
	unsigned long flags; 
	int on_rq, running, oldprio;

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

	p->prio_per_cpu = 1;
	p->smp_prio = (char*) smp_prio ;
	//__setscheduler(rq,p,1,smp_prio[rq->cpu]);

	if (running)
		p->sched_class->set_curr_task(rq);
	if (on_rq)
		enqueue_task(rq, p, ENQUEUE_HEAD );

	task_rq_unlock(rq,p,&flags);
	return 0;
}

