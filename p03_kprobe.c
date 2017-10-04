#include "p03_module.h"
#include <linux/kprobes.h>
#include <config/x86/tsc.h>

/* enqueueing kprobe */
#define MAX_SYMBOL_LEN	64
static char symbol_en[MAX_SYMBOL_LEN] = "FIGURE THIS OUT";
module_param_string(symbol_en, symbol_en, sizeof(symbol_en), 0644);


static struct kprobe kp_en = {
    .symbol_name    = symbol_en,
};


/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_en_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(PRINT_PREF "kprobe_en_pre worked %llu\n", rdtsc());
    return 0;
}

/* kprobe (post_handler: called after the probed instruction is executed */
static void handler_en_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
    printk(PRINT_PREF "kprobe_en_post worked\n");
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_en_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(PRINT_PREF "fault_en_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


/* dequeueing kprobe */

static char symbol_de[MAX_SYMBOL_LEN] = "FIGURE THIS OUT";
module_param_string(symbol_de, symbol_de, sizeof(symbol_de), 0644);

static struct kprobe kp_de = {
    .symbol_name    = symbol_de,
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_de_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(PRINT_PREF "kprobe_de_pre worked %llu\n", rdtsc());
    return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_de_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
    printk(PRINT_PREF "kprobe_de_post worked\n");
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_de_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(PRINT_PREF "fault_de_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


/* functions for main to call to insert and remove probe */
int ins_probe(void) {
    int ret;
    kp_en.pre_handler = handler_en_pre;
	kp_en.post_handler = handler_en_post;
	kp_en.fault_handler = handler_en_fault;
   
    printk(PRINT_PREF "%s,  %s", symbol_en, symbol_de); 
    ret = register_kprobe(&kp_en);
	if (ret < 0) {
		pr_err(PRINT_PREF "register_kprobe_en failed, returned %d\n", ret);
		return ret;
	}
	pr_info(PRINT_PREF "Planted kprobe_en at %p\n", kp_en.addr);
    
    kp_de.pre_handler = handler_de_pre;
	kp_de.post_handler = handler_de_post;
	kp_de.fault_handler = handler_de_fault;
	
    ret = register_kprobe(&kp_de);
	if (ret < 0) {
		pr_err(PRINT_PREF "register_kprobe_de failed, returned %d\n", ret);
		return ret;
	}
	pr_info(PRINT_PREF "Planted kprobe_de at %p\n", kp_de.addr);
    return 0;
}

void rm_probe(void) {
    unregister_kprobe(&kp_en);
    unregister_kprobe(&kp_de);
    pr_info("kprobe_en at %p unregistered\n", kp_en.addr);
    pr_info("kprobe_de at %p unregistered\n", kp_de.addr);
}
