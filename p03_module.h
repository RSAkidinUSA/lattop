#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/seq_file.h>

#define PRINT_PREF  KERN_INFO "[lattop]: "
#define PRINT_DONE  printk(PRINT_PREF "Done.\n")

int ins_probe(void);
void rm_probe(void);

/* rb tree functions */
int rb_init(void);
int set_asleep(pid_t pid, unsigned long long time);
void set_awake(pid_t pid, unsigned long long time);
void print_rb(void);
void print_rb_proc(struct seq_file *m);
void rb_free(void);

/* proc functions */
int proc_open(void);
void proc_close(void);
