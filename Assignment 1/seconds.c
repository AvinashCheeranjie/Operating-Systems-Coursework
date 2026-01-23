/**
From 'lsb_release -a' command, Ubuntu distribution

No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 22.04.5 LTS
Release:	22.04
Codename:	jammy

From 'uname -r' command, kernel version

6.8.0-51-generic
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



// Function prototypes, variables, and the like

ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);

static const struct proc_ops my_proc_ops = {
        .proc_read = proc_read,
};

unsigned long my_jiffies;
unsigned long my_HZ;



// This function loads kernel module, queries for and saves the system's HZ value and current jiffies value in global variables.
// It also creates a /proc file named /proc/seconds when the kernel module is loaded.
int proc_init(void)
{
	
	// Debugging messages are printed in dmesg, TA can ignore all printk() statements in this file
	printk(KERN_INFO "Loading Module\n");
    	printk(KERN_INFO "jiffies = %lu\n",jiffies);
	printk(KERN_INFO "HZ = %d\n",HZ);

	my_jiffies = jiffies;
    	my_HZ = HZ;
        proc_create(PROC_NAME, 0, NULL, &my_proc_ops);
        
        printk(KERN_INFO "/proc/%s created\n", PROC_NAME);

	return 0;
}


// This function removes the kernel module and the /proc/seconds file.
void proc_exit(void) {

	printk(KERN_INFO "Removing Module\n");

        remove_proc_entry(PROC_NAME, NULL);

        printk( KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

 
// This function is executed whenever the file /proc/seconds is read.
// When the command 'cat /proc/seconds' is called, it prints the number of seconds elapsed since the kernel module was loaded as an integer number.
// Note: The number of elapsed seconds is reported as an integer number, as printk cannot handle floats.
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0; // Logic to ensure the proc_read() function exits properly
        unsigned long time_elapsed = 0; // Initialized to 0, if 0 is reported then something is not working (assuming you take more than 1 second to run the cat /proc/seconds command).

        if (completed) {
                completed = 0;
                return 0;
        }

	printk(KERN_INFO "jiffies at read time = %lu\n",jiffies);
	printk(KERN_INFO "time elapsed since kernel module loaded = %lu\n",((jiffies-my_jiffies)/my_HZ));
	
	time_elapsed = ((jiffies-my_jiffies)/my_HZ); // Calculates time elapsed
	
        completed = 1;

        rv = sprintf(buffer, "%lu\n", time_elapsed); // Writes to kernel buffer
        copy_to_user(usr_buf, buffer, rv); // Writes to user buffer

        return rv;

}


/* Macros for registering module entry and exit points. */
module_init( proc_init );
module_exit( proc_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seconds Module");
MODULE_AUTHOR("Wilson Liu & Andy Liang");

