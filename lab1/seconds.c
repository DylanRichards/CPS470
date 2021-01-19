/**
 * seconds.c
 *
 * Kernel module that prints the number of elapsed seconds since the kernel module was loaded when /proc/seconds file is read
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <asm/param.h>


#define PROC_NAME "seconds"
#define BUFFER_SIZE 128

static ssize_t my_proc_read(struct file *file, char *buf, size_t count, loff_t *pos);

static struct proc_ops my_ops = {
        .proc_read = my_proc_read
};

long long unsigned int start_jiffies;


/* This function is called when the module is loaded. */
static int proc_init(void)
{
        // creates the /proc/seconds entry
        proc_create(PROC_NAME, 0, NULL, &my_ops);
        start_jiffies = get_jiffies_64();
	return 0;
}

/* This function is called when the module is removed. */
static void proc_exit(void) {
        // removes the /proc/seconds entry
        remove_proc_entry(PROC_NAME, NULL);
}

/**
 * This function is called each time the /proc/seconds is read.
 */
static ssize_t my_proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        long long unsigned int end_jiffies = get_jiffies_64();
        int elapsed;

        int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0;

        if (completed) {
                completed = 0;
                return 0;
        }

        completed = 1;

        elapsed = (end_jiffies - start_jiffies) / HZ;
        rv = sprintf(buffer, "Elapsed time: %d seconds\n", elapsed);

        // copies the contents of buffer to userspace usr_buf
        copy_to_user(usr_buf, buffer, rv);

        return rv;
}


/* Macros for registering module entry and exit points. */
module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Prints the number of elapsed seconds since the kernel module was loaded when /proc/seconds file is read");
MODULE_AUTHOR("Dylan Richards");
