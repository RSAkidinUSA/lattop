#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

#define PRINT_PREF  KERN_INFO "[lattop]: "
#define PRINT_DONE  printk(PRINT_PREF "Done.\n")

int ins_probe(void);
void rm_probe(void);
