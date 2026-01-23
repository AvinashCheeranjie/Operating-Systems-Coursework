/**
 * Linux Distribution:
 * No LSB modules are available.
 * Distributor ID: Ubuntu
 * Description: Ubuntu 25.10
 * Release: 25.10
 * Codename: questing
 *
 * Kernel Version: 6.17.0-5-generic
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/gcd.h>
#include <asm/param.h>
#include <linux/sched.h>

#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

// Function prototypes
ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);
static const struct proc_ops my_proc_ops = {
        .proc_read = proc_read,
};

// Global variables
unsigned long my_jiffies;
unsigned long my_HZ;

/* This function is called when the module is loaded.
 * It saves the system's HZ and current jiffies values and creates the /proc/seconds entry
 */
int proc_init(void)
{
	printk(KERN_INFO "Loading Module\n");
    	printk(KERN_INFO "jiffies: %lu\n",jiffies);
	printk(KERN_INFO "HZ: %d\n",HZ);

	my_jiffies = jiffies;
    	my_HZ = HZ;

        proc_create(PROC_NAME, 0, NULL, &my_proc_ops);
        printk(KERN_INFO "/proc/%s created\n\n", PROC_NAME);

	return 0;
}

/* This function is called when the module is removed. */
void proc_exit(void)
{
	printk(KERN_INFO "Removing Module\n");
        remove_proc_entry(PROC_NAME, NULL);
        printk( KERN_INFO "/proc/%s removed\n\n", PROC_NAME);
}

/* This function is executed when /proc/seconds is read.
 * It prints the integer number of seconds elapsed since the kernel module was loaded when 'cat /proc/seconds' is called
 */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
	int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0; // flag to ensure the function exits properly
        unsigned long time_elapsed = 0;

        if (completed){
		completed = 0;
		return 0;
	}

	printk(KERN_INFO "Jiffies at read time: %lu\n", jiffies);
	printk(KERN_INFO "Time elapsed since kernel module loaded: %lu s\n\n", ((jiffies - my_jiffies)/my_HZ));

	time_elapsed = ((jiffies - my_jiffies)/my_HZ);
	completed = 1;

        rv = sprintf(buffer, "Time elapsed since module loaded: %lu s\n", time_elapsed); // write to kernel buffer
        copy_to_user(usr_buf, buffer, rv); // write to user buffer

        return rv;
}

/* Macros for registering module entry and exit points. */
module_init( proc_init );
module_exit( proc_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seconds");
MODULE_AUTHOR("Avinash Cheeranjie and Habel Kingson");
