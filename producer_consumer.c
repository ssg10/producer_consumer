/*
===============================================================================
Driver Name		:		producer_consumer
Author			:		SSG
License			:		GPL
Description		:		producer and consumer framework code
                        consumer thread and producer thread communicating
                        to each others.
                        producer gives job TO consumer to do via linked list
                        every 30 secs.
                        this can be used to demonstrate processes and linkedlist
                        in linux or used as a framework for further development.
===============================================================================
*/

#include"producer_consumer.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSG");


static struct task_struct *producer;
static struct task_struct *consumer;
static spinlock_t list_lock;

struct task_list{
	char task_name[10];
	struct list_head list;
};

struct task_list mytask_list;

static int thread_producer_entry(void* thread_data)
{
	int delay = 30*HZ;
	struct task_list *tmp_task_list;

	PINFO(" Producer entry\n");

	/* Keep thread spinning until driver unload stops the thread, exit loop,
	 * and thread terminates itself by return 0
	 *  */
	while(!kthread_should_stop()){
		PINFO("Inside the spinning while loop in thread\n");

		/* Feed the consumer with task */
		spin_lock(&list_lock);

		/* Add new task into the list */
		tmp_task_list = (struct task_list *)kmalloc(sizeof(struct task_list), GFP_KERNEL);
		strcpy(tmp_task_list->task_name,"cleanroom");
		list_add_tail(&tmp_task_list->list, &(mytask_list.list));

		/* Add another new task into the list */
		tmp_task_list = (struct task_list *)kmalloc(sizeof(struct task_list), GFP_KERNEL);
		strcpy(tmp_task_list->task_name,"washdish");
		list_add_tail(&tmp_task_list->list, &(mytask_list.list));

		/* Add new task into the list */
		tmp_task_list = (struct task_list *)kmalloc(sizeof(struct task_list), GFP_KERNEL);
		strcpy(tmp_task_list->task_name,"buyfood");
		list_add_tail(&tmp_task_list->list, &(mytask_list.list));

		spin_unlock(&list_lock);

		wake_up_process(consumer);

		/* Set it to wake up every 30 secs to add tasks */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(delay);

		set_current_state(TASK_RUNNING);
	}

	PINFO(" Exit: Producer thread terminates\n");

	return 0;
}

static int thread_consumer_entry(void* thread_data)
{
	struct list_head *pos, *q;
	struct task_list *tmp_task_list;

	PINFO(" Consumer entry\n");

	while(!kthread_should_stop()){
		PINFO("Inside the spinning while loop in thread\n");

		/*RACE CONDITION SOLUTION. Move set current state to here.*/
		/*How this solves it? We have changed our current state to TASK_INTERRUPTIBLE,
		before we test the list_empty condition. So, what has changed?
		The change is that whenever a wake_up_process is called
		for a process whose state is TASK_INTERRUPTIBLE or TASK_UNINTERRUPTIBLE,
		and the process has not yet called schedule(),
		the state of the process is changed back to TASK_RUNNING.

		Thus, even if a wake-up is delivered by process producer
		at any point after the check for list_empty is made, the state of consumer automatically
		is changed to TASK_RUNNING. Hence, the call to schedule() does not put
		process consumer to sleep; it merely schedules it out for a while.
		Thus, the wake-up no longer is lost. */

		set_current_state(TASK_INTERRUPTIBLE);
		spin_lock(&list_lock);

		if(list_empty(&mytask_list.list)) {
			spin_unlock(&list_lock);
			/* RACE CONDITION EXAMPLE: Right here, if producer at another core grabs lock,
			 * add tasks to list, and call wake_up_process, before consumer set state
			 * to interruptible below, then the wake up is lost. Consumer will sleep.
			 * (Of course, in this example, producer will wake up consumer again every
			 * 30 secs. Then, consumer will process the tasks, eventually. - however,
			 * it has to wait 30 secs. Or never, if the producer is designed not to wake up
			 * consumer at all.)
			 *
			 * In this timeslice, producer executes all its instructions.
			 * Thus, it performs a wake-up on consumer, which has not yet gone to sleep.
			 * Now, consumer, wrongly assuming that it safely has performed the check for list_empty,
			 * sets the state to TASK_INTERRUPTIBLE and goes to sleep. */

			/*set_current_state(TASK_INTERRUPTIBLE);*/
			schedule();
		}
		/* List not empty. Pick up the task and process it */
		else {
			PINFO(" There are tasks in the list and pick them up\n");

			list_for_each_safe(pos, q, &mytask_list.list) {
				tmp_task_list = list_entry(pos, struct task_list, list);
				PINFO("Picked up task = %s\n", tmp_task_list->task_name);
				/* erase the node */
				list_del(pos);
			}

			/* Check what is inside the list now - do not expect this snippet
			 * to be executed */
			list_for_each(pos, &mytask_list.list){
				tmp_task_list = list_entry(pos, struct task_list, list);
				PINFO("Traversing list and found: %s\n", tmp_task_list->task_name);
			}
			spin_unlock(&list_lock);

			//set_current_state(TASK_INTERRUPTIBLE);  // no need to redo set state here
			schedule();
		}

		PINFO("Fall off list_empty check\n");

		/* See note at the bottom. Here, a wake_up_process from producer will change consumer
		 * thread back to TASK_RUNNING. Back to the top of while loop.
		 * No need to call set_current_state(TASK_RUNNING) here.
		 */
	}

	PINFO(" Exit: Consumer thread terminates");

	return 0;
}

static int __init producer_consumer_init(void)
{
	PINFO("DRIVER INIT\n");

	/* Init the list in the task_list struct */
	INIT_LIST_HEAD(&mytask_list.list);

    producer = kthread_create(thread_producer_entry,NULL,"myproducer");

    if (IS_ERR(producer)) {
       return PTR_ERR(producer);
    };

    consumer = kthread_create(thread_consumer_entry,NULL,"myconsumer");

    if (IS_ERR(consumer)) {
       return PTR_ERR(consumer);
    }

    spin_lock_init(&list_lock);

    /* wake up the processes, creating them does not mean processes will run automatically */
    /* or, use kthread_run() instead of kthread_create() */
    wake_up_process(producer);
    wake_up_process(consumer);

	return 0;
}

static void __exit producer_consumer_exit(void)
{	
	 int ret;

	 ret = kthread_stop(producer);
	 if(!ret)
		 PINFO(" producer thread stopped\n");

	 /* kthread_stop() actually calls wake_up_process().
	  * This is why the thread function can exit when polling kthread_should_stop() in loop.
	  */
	 ret = kthread_stop(consumer);
	 if(!ret)
		 PINFO(" consumer thread stopped\n");

	PINFO("DRIVER EXIT\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);

/* NOTE: whenever a wake_up_process is called for a process whose state is TASK_INTERRUPTIBLE or
TASK_UNINTERRUPTIBLE, and the process has not yet called schedule(), the state of the process
is changed back to TASK_RUNNING.
*/

// OUTPUT shows that race condition does occur! At 627644.045307, consumer picked up previous lost wakeup call.
//May 27 18:24:49 ubuntu kernel: [627614.010580] producer_consumer_init:DRIVER INIT
//May 27 18:24:49 ubuntu kernel: [627614.010616] thread_producer_entry: Producer entry
//May 27 18:24:49 ubuntu kernel: [627614.010617] thread_consumer_entry: Consumer entry
//May 27 18:24:49 ubuntu kernel: [627614.010618] thread_consumer_entry:Inside the spinning while loop in thread
//May 27 18:24:49 ubuntu kernel: [627614.010620] thread_producer_entry:Inside the spinning while loop in thread
//May 27 18:25:19 ubuntu kernel: [627644.045238] thread_producer_entry:Inside the spinning while loop in thread
//May 27 18:25:19 ubuntu kernel: [627644.045294] thread_consumer_entry:Inside the spinning while loop in thread
//May 27 18:25:19 ubuntu kernel: [627644.045303] thread_consumer_entry: There are tasks in the list and pick them up
//May 27 18:25:19 ubuntu kernel: [627644.045307] thread_consumer_entry:Picked up task = cleanroom
//May 27 18:25:19 ubuntu kernel: [627644.045311] thread_consumer_entry:Picked up task = washdish
//May 27 18:25:19 ubuntu kernel: [627644.045314] thread_consumer_entry:Picked up task = buyfood
//May 27 18:25:19 ubuntu kernel: [627644.045317] thread_consumer_entry:Picked up task = cleanroom
//May 27 18:25:19 ubuntu kernel: [627644.045320] thread_consumer_entry:Picked up task = washdish
//May 27 18:25:19 ubuntu kernel: [627644.045322] thread_consumer_entry:Picked up task = buyfood
//May 27 18:25:49 ubuntu kernel: [627674.081127] thread_producer_entry:Inside the spinning while loop in thread
//May 27 18:25:49 ubuntu kernel: [627674.081181] thread_consumer_entry:Inside the spinning while loop in thread
//May 27 18:25:49 ubuntu kernel: [627674.081190] thread_consumer_entry: There are tasks in the list and pick them up
//May 27 18:25:49 ubuntu kernel: [627674.081194] thread_consumer_entry:Picked up task = cleanroom
//May 27 18:25:49 ubuntu kernel: [627674.081197] thread_consumer_entry:Picked up task = washdish
//May 27 18:25:49 ubuntu kernel: [627674.081200] thread_consumer_entry:Picked up task = buyfood
