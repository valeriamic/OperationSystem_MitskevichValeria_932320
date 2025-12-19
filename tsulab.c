#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/version.h>

#define PROCFS_NAME "tsulab"

#define HALE_BOPP_PERIHELION_EPOCH_SEC 859334400 

#define SECONDS_IN_DAY (24 * 60 * 60)

static struct proc_dir_entry *our_proc_file;

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset)
{
    char s[32];
    int len = 0;

    if (*offset > 0) {
        return 0;
    }

    struct timespec64 current_time;
    ktime_get_real_ts64(&current_time);
    
    long long diff_sec = current_time.tv_sec - HALE_BOPP_PERIHELION_EPOCH_SEC;

    long long days = diff_sec / SECONDS_IN_DAY;
    
    len = snprintf(s, sizeof(s), "%lld\n", days);

    if (len > buffer_length) {
        return -EINVAL;
    }

    if (copy_to_user(buffer, s, len)) {
        return -EFAULT;
    }

    *offset += len;
    
    pr_info("tsulab: /proc/%s read. Days calculated: %lld\n", PROCFS_NAME, days);
    
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init tsulab_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");

    our_proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_fops);

    if (our_proc_file == NULL) {
        remove_proc_entry(PROCFS_NAME, NULL);
        pr_alert("tsulab: Error: Could not create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    
    pr_info("tsulab: /proc/%s created successfully.\n", PROCFS_NAME);
    
    return 0;
}

static void __exit tsulab_exit(void)
{
    remove_proc_entry(PROCFS_NAME, NULL);
    pr_info("tsulab: /proc/%s removed.\n", PROCFS_NAME);
    
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MitskevichValeria");
MODULE_DESCRIPTION("TSU Linux Kernel Lab Module.");