#include "p03_module.h"
#include <linux/proc_fs.h>

struct proc_dir_entry *lattop_proc;
static const char _name[] = "lattop";

static int lattop_proc_show(struct seq_file *m, void *v) {
    print_rb_proc(m);
    return 0;
}

static int lattop_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, lattop_proc_show, NULL);
}

static const struct file_operations lattop_operations = {
    .owner = THIS_MODULE,
    .open = lattop_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

int proc_open(void) {
    lattop_proc = proc_create(_name, 0, NULL, &lattop_operations);
    if (lattop_proc == NULL) {
        remove_proc_entry(_name, NULL);
        return -ENOMEM;
    }
    return 0;
}

void proc_close(void) {
    remove_proc_entry(_name, NULL);
}
