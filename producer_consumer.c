/*
===============================================================================
Driver Name		:		producer_consumer
Author			:		SSG
License			:		GPL
Description		:		producer and consumer framework code
                        consumer thread and producer thread communicating
                        to each others.
                        producer gives job TO consumer to do via linked list
===============================================================================
*/

#include"producer_consumer.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSG");


static struct task_struct *producer;
static struct task_struct *consumer;


struct mylist_node {
	struct list_head list;

};
struct list_head mylist_head;	

static int thread_producer_entry(void* thread_data)
{
	PINFO(" Entry\n");

	/* Keep thread spinning until driver unload stop the thread */
	while(!kthread_should_stop()){
		PINFO("Inside the spinning while loop in thread\n");
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);
	}

	PINFO(" Exit: Producer thread terminates\n");

	return 0;
}

static int thread_consumer_entry(void* thread_data)
{
	PINFO(" Entry\n");

	while(!kthread_should_stop()){
		PINFO("Inside the spinning while loop in thread\n");
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);
	}

	PINFO(" Exit: Consumer thread terminates");

	return 0;
}

static int __init producer_consumer_init(void)
{


	PINFO("DRIVER INIT\n");

	INIT_LIST_HEAD(&mylist_head);

    producer = kthread_create(thread_producer_entry,NULL,"myproducer");

    if (IS_ERR(producer)) {
       return PTR_ERR(producer);
    };

    consumer = kthread_create(thread_consumer_entry,NULL,"myconsumer");

    if (IS_ERR(consumer)) {
       return PTR_ERR(consumer);
    }

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

	 ret = kthread_stop(consumer);
	 if(!ret)
		 PINFO(" consumer thread stopped\n");

	PINFO("DRIVER EXIT\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);

