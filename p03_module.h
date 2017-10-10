#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>

#define PRINT_PREF  KERN_INFO "[lattop]: "
#define PRINT_DONE  printk(PRINT_PREF "Done.\n")

int ins_probe(void);
void rm_probe(void);

/* rb tree functions */
int rb_init(void);
int set_asleep(pid_t pid, long long time);
int set_awake(pid_t pid, long long time);
void print_rb(void);
void rb_free(void);
