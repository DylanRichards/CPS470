/**
 * jiffies.c
 *
 * Kernel module that prints current value of jiffies when /proc/jiffies file is read
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS
#endif

#define PROC_NAME "jiffies"
#define BUFFER_SIZE 128

ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);

#ifdef HAVE_PROC_OPS
static const struct proc_ops jiffies_ops = {
        .proc_read = proc_read,
};
#else
static const struct file_operations jiffies_ops = {
        .owner = THIS_MODULE,
        .read = proc_read,
};
#endif

/* This function is called when the module is loaded. */
static int proc_init(void)
{
        // creates the /proc/jiffies entry
        proc_create(PROC_NAME, 0, NULL, &jiffies_ops);
        return 0;
}

/* This function is called when the module is removed. */
static void proc_exit(void)
{
        // removes the /proc/jiffies entry
        remove_proc_entry(PROC_NAME, NULL);
}

/**
 * This function is called each time the /proc/jiffies is read.
 */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0;

        if (completed) {
                completed = 0;
                return 0;
        }

        completed = 1;

        rv = sprintf(buffer, "Jiffies: %llu\n", get_jiffies_64());

        // copies the contents of buffer to userspace usr_buf
        raw_copy_to_user(usr_buf, buffer, rv);

        return rv;
}

/* Macros for registering module entry and exit points. */
module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Prints current value of jiffies when /proc/jiffies file is read");
MODULE_AUTHOR("Dylan Richards");
