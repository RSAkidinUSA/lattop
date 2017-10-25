#include "p03_module.h"

static int __init p03_init(void) {
    int ret = 0;
    /* open the proc fs */
    ret = proc_open();
    if (ret) {
        goto proc_fail;
    }
    /* intialize the red_black tree */
    ret = rb_init();
    if (ret) {
        goto init_fail;
    }
    /* insert the kprobe */
    ret = ins_probe();
    if (ret) {
        goto probe_fail;
    }
    printk(PRINT_PREF "Latency Profiler Module loaded...\n");
    return 0;

probe_fail:
    rb_free();
init_fail:
    proc_close();
proc_fail:
    printk(PRINT_PREF "Latency Profiler Module failed to load...\n");
    return ret;
}

static void __exit p03_exit(void) {
    /* Remove the kprobe */
    rm_probe();
    /* free the rb tree */
    rb_free();
    /* close the proc fs */
    proc_close();
    printk(PRINT_PREF "Latency module unloaded...\n");
}


module_init(p03_init);
module_exit(p03_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ryan Burrow <rsardb11@vt.edu>");
MODULE_DESCRIPTION("Kernel module for ECE 4984 Project 3");
