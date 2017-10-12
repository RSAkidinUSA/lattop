#include "p03_module.h"
#include <linux/timer.h>

int g_time_interval = 10000;
struct timer_list g_timer;

void _TimerHandler(unsigned long data) {
    /*Restarting the timer...*/
    mod_timer( &g_timer, jiffies + msecs_to_jiffies(g_time_interval));
 
    /* print_rb(); */
}

static int __init p03_init(void) {
    int ret;
    printk(PRINT_PREF "Latency Profiler Module loaded...\n");
    /* open the proc fs */
    ret = proc_open();
    if (ret) {
        return ret;
    }
    /* intialize the red_black tree */
    ret = rb_init();
    if (ret) {
        return ret;
    }
    /* insert the kprobe */
    ret = ins_probe();
    if (ret) {
        rb_free();
        return ret;
    }
    
    /* Start a 10s countdown timer */
    setup_timer(&g_timer, _TimerHandler, 0);
    mod_timer( &g_timer, jiffies + msecs_to_jiffies(g_time_interval));


    return 0;
}

static void __exit p03_exit(void) {
    /* stop the timer */
    del_timer(&g_timer);
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
