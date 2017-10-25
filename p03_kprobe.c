#include "p03_module.h"
#include <linux/kprobes.h>
#include <config/x86/tsc.h>

/* enqueueing kprobe */
#define MAX_SYMBOL_LEN	64
static char symbol_wake[MAX_SYMBOL_LEN] = "activate_task";
module_param_string(symbol_wake, symbol_wake, sizeof(symbol_wake), 0644);

static struct kprobe kp_wake = {
    .symbol_name    = symbol_wake,
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_wake_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct task_struct *ts;
    struct lat_data ld;
    long long unsigned time = rdtsc();
    ts = (struct task_struct *)regs->si;
    /* set latency data */
    ld.pid = ts->pid;
    ld.time = time;
    set_awake(&ld);
    return 0;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_wake_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(PRINT_PREF "fault_wake_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


/* dequeueing kprobe */

static char symbol_sleep[MAX_SYMBOL_LEN] = "deactivate_task";
module_param_string(symbol_sleep, symbol_sleep, sizeof(symbol_sleep), 0644);

static struct kprobe kp_sleep = {
    .symbol_name    = symbol_sleep,
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_sleep_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct task_struct *ts;
    struct lat_data ld;
    unsigned long entries[STACK_DEPTH];
    struct stack_trace st = {
        .nr_entries = 0,
        .entries = &entries[0],
        .max_entries = STACK_DEPTH,
        .skip = 0,
    };
    long long unsigned time = rdtsc();
    ts = (struct task_struct *)regs->si;
    ld.pid = ts->pid;
    strncpy(ld.name, ts->comm, TASK_COMM_LEN);
    if (ts->mm) {
        save_stack_trace_user(&st);
    } else {
        save_stack_trace(&st);
    }
    ld.time = time;
    ld.st = &st;
    set_asleep(&ld);
    return 0;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_sleep_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(PRINT_PREF "fault_de_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


/* functions for main to call to insert and remove probe */
int ins_probe(void) {
    int ret;
    kp_wake.pre_handler = handler_wake_pre;
	kp_wake.fault_handler = handler_wake_fault;
   
    ret = register_kprobe(&kp_wake);
	if (ret < 0) {
		pr_err(PRINT_PREF "register_kprobe_wake failed, returned %d\n", ret);
		return ret;
	}
	pr_info(PRINT_PREF "Planted kprobe_wake at %p\n", kp_wake.addr);
    
    kp_sleep.pre_handler = handler_sleep_pre;
	kp_sleep.fault_handler = handler_sleep_fault;
	
    ret = register_kprobe(&kp_sleep);
	if (ret < 0) {
		pr_err(PRINT_PREF "register_kprobe_sleep failed, returned %d\n", ret);
		return ret;
	}
	pr_info(PRINT_PREF "Planted kprobe_sleep at %p\n", kp_sleep.addr);
    
    return 0;
}

void rm_probe(void) {
    unregister_kprobe(&kp_wake);
    unregister_kprobe(&kp_sleep);
    pr_info("kprobe_en at %p unregistered\n", kp_wake.addr);
    pr_info("kprobe_de at %p unregistered\n", kp_sleep.addr);
}
