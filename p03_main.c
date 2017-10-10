#include "p03_module.h"

static int __init p03_init(void) {
    int ret;
    printk(PRINT_PREF "Latency Profiler Module loaded...\n");
    ret = rb_init();
    if (ret) {
        return ret;
    }
    /* focus on getting the other functionality working first, then kprobe */
    ret = ins_probe();
    if (ret) {
        rb_free();
        return ret;
    }
    return 0;
}

static void __exit p03_exit(void) {
    /* focus on getting the other functionality working first, then kprobe */
    rm_probe();
    rb_free();
    printk(PRINT_PREF "Latency module unloaded...\n");
}


module_init(p03_init);
module_exit(p03_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ryan Burrow <rsardb11@vt.edu>");
MODULE_DESCRIPTION("Kernel module for ECE 4984 Project 3");
