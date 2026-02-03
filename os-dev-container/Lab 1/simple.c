/**
 * simple.c
 *
 * A simple kernel module. 
 * 
 * To compile, run makefile by entering "make"
 *
 * Operating System Concepts - 10th Edition
 * Copyright John Wiley & Sons - 2018
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>

/* Prints the required members of init_task struct  */
void print_init_PCB(void)
{
	printk(KERN_INFO "init_task pid: %d\n",init_task.pid);
	printk(KERN_INFO "init_task state: %ld\n",init_task.__state);
	printk(KERN_INFO "init_task flags: %u\n",init_task.flags);
	printk(KERN_INFO "init_task runtime policy: %ld\n",init_task.rt_priority);
	printk(KERN_INFO "init_task process policy: %u\n",init_task.policy);
	printk(KERN_INFO "init_task task group id: %d\n",init_task.tgid);
}


/* This function is called when the module is loaded. */
static int simple_init(void)
{
	printk(KERN_INFO "Loading Module\n");
	print_init_PCB();
	return 0;
}

/* This function is called when the module is removed. */
static void simple_exit(void) {
	printk(KERN_INFO "Removing Module\n");
}

/* Macros for registering module entry and exit points. */
module_init( simple_init );
module_exit( simple_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");

