#include "p03_module.h"

#include <linux/kprobes.h>

#define MAX_SYMBOL_LEN	64
static char symbol[MAX_SYMBOL_LEN] = "do_exit";
module_param_string(symbol, symbol, sizeof(symbol), 0644);

static struct kprobe kp = {
    .symbol_name    = symbol,
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(PRINT_PREF "kprobe_pre worked\n");
    return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
    printk(PRINT_PREF "kprobe_post worked\n");
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(PRINT_PREF "fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


/* functions for main to call to insert and remove probe */
int ins_probe(void) {
    int ret;
    kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err(PRINT_PREF "register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info(PRINT_PREF "Planted kprobe at %p\n", kp.addr);
    return 0;
}

void rm_probe(void) {
    unregister_kprobe(&kp);
    pr_info("kprobe at %p unregistered\n", kp.addr);
}
