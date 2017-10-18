#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>
#include <linux/uaccess.h>

DEFINE_RWLOCK(hash_lock);

/* Hashtable code */

struct st_slot {
    struct hlist_node hash_node;
    struct stack_trace *s_t, *s_t_user;
    u32 hash;
    unsigned long long sleep_time;
};

/* custom snprint_stack_trace to ignore function offsets */
static int __snprint_stack_trace(char *buf, size_t size,
			struct stack_trace *trace, int spaces)
{
	int i;
	int generated;
	int total = 0;

	if (WARN_ON(!trace->entries))
		return 0;

	for (i = 0; i < trace->nr_entries; i++) {
		generated = snprintf(buf, size, "%*c%ps\n", 1 + spaces, ' ',
				     (void *)trace->entries[i]);

		total += generated;

		/* Assume that generated isn't a negative number */
		if (generated >= size) {
			buf += size;
			size = 0;
		} else {
			buf += generated;
			size -= generated;
		}
	}

	return total;
}

/*
void (*sstu)(struct stack_trace *trace) = (void (*)(struct stack_trace *)) 0xffffffff8da3de40;

void save_stack_trace_user(struct stack_trace *trace) {
    sstu(trace);
}
*/
/* Copied save_stack_trace_user code */
struct stack_frame_user {
	const void __user	*next_fp;
	unsigned long		ret_addr;
};

static int
copy_stack_frame(const void __user *fp, struct stack_frame_user *frame)
{
	int ret;

	if (!access_ok(VERIFY_READ, fp, sizeof(*frame)))
		return 0;

	ret = 1;
	pagefault_disable();
	if (__copy_from_user_inatomic(frame, fp, sizeof(*frame)))
		ret = 0;
	pagefault_enable();

	return ret;
}

static inline void __save_stack_trace_user(struct stack_trace *trace)
{
	const struct pt_regs *regs = task_pt_regs(current);
	const void __user *fp = (const void __user *)regs->bp;

	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = regs->ip;

	while (trace->nr_entries < trace->max_entries) {
		struct stack_frame_user frame;

		frame.next_fp = NULL;
		frame.ret_addr = 0;
		if (!copy_stack_frame(fp, &frame))
			break;
		if ((unsigned long)fp < regs->sp)
			break;
		if (frame.ret_addr) {
			trace->entries[trace->nr_entries++] =
				frame.ret_addr;
		}
		if (fp == frame.next_fp)
			break;
		fp = frame.next_fp;
	}
}

void save_stack_trace_user(struct stack_trace *trace)
{
	/*
	 * Trace user stack if we are not a kernel thread
	 */
	if (current->mm) {
		__save_stack_trace_user(trace);
	}
	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}


#define __BUF_SIZE 1024
/* hash a stack trace using jhash */
static u32 __hash_st (struct stack_trace *s_t) {
    int buflen;
    char buf[__BUF_SIZE];
    buflen = __snprint_stack_trace(buf, __BUF_SIZE, s_t, 0);
    return jhash((void *)buf, buflen * sizeof(char), JHASH_INITVAL);
}

/* init a new stack trace with data from a given one */
static struct stack_trace *__init_st(struct stack_trace *src) {
    struct stack_trace *temp;
    int i = 0;
    temp = kmalloc(sizeof(*temp), GFP_ATOMIC);
    if (temp == NULL) {
        goto err_st;
    }
    temp->entries = kmalloc(sizeof(unsigned long) * STACK_DEPTH, GFP_ATOMIC);
    if (temp->entries == NULL) {
        goto err_entries;
    }
    temp->max_entries = STACK_DEPTH;
    temp->nr_entries = src->nr_entries;
    for (i = 0; i < STACK_DEPTH; i++) {
        temp->entries[i] = src->entries[i];
    }
    return temp;

err_entries:
    kfree(temp);
err_st:
    return temp;
}

/* add a trace to the hash table or update it if it exists */
void add_trace(struct lat_data *ld, struct taskNode *tn) {
    u32 st_hash;
    bool found = false;
    struct st_slot *temp_slot;
    struct stack_trace *temp_st, *temp_st_user;
    st_hash = __hash_st(ld->s_t);
    read_lock(&hash_lock);
    hash_for_each_possible(tn->st_ht, temp_slot, hash_node, st_hash) {
        /* stack trace is already in the table */ 
        if (st_hash == temp_slot->hash) {
            found = true;
            break;
        }
    }
    read_unlock(&hash_lock);
    if (!found) {
        /* stack trace is not in the table  */
        temp_slot = kmalloc(sizeof(*temp_slot), GFP_ATOMIC);
        if (temp_slot == NULL) {
            goto no_slot_mem;
        }
        temp_st = __init_st(ld->s_t);
        if (temp_st == NULL) {
            goto no_st_mem;
        }
        temp_st_user = __init_st(ld->s_t_user);
        if (temp_st_user == NULL) {
            goto no_st_user_mem;
        }
        temp_slot->sleep_time = 0;
        temp_slot->s_t = temp_st;
        temp_slot->s_t_user = temp_st_user;
        temp_slot->hash = st_hash;
        write_lock(&hash_lock);
        hash_add(tn->st_ht, &temp_slot->hash_node, temp_slot->hash);
        write_unlock(&hash_lock);
    }
    temp_slot->sleep_time += (ld->time - tn->start_sleep);
    return;

no_st_user_mem:
    kfree(temp_st);
no_st_mem:
    kfree(temp_slot);
no_slot_mem:
    printk(PRINT_PREF "unable to save stack trace info, no memory left\n");
    return; 


}

static void __seqprint_stack_trace(struct seq_file *m, struct stack_trace *trace) {
    int i;

    if (WARN_ON(!trace->entries)) {
        return;
    }

    for (i = 0; i < trace->nr_entries; i++) {
        if (trace->entries[i] != ULONG_MAX) {
            seq_printf(m, "%*c%pS\n", 1, ' ', (void *)trace->entries[i]);
        }
    }
}

/* print the table of a given task */
void print_table(struct seq_file *m, struct taskNode *tn) {
    int bkt;
    struct st_slot *temp_slot, *high_lat;
    high_lat = NULL;
    read_lock(&hash_lock);
    hash_for_each(tn->st_ht, bkt, temp_slot, hash_node) {
        if (high_lat == NULL) {
            high_lat = temp_slot;
        } else if (high_lat->sleep_time < temp_slot->sleep_time) {
            high_lat = temp_slot;
        }
    }
    read_unlock(&hash_lock);
    if (high_lat != NULL) {
        seq_printf(m, "Max stack trace latency: %-15llu\n", \
                high_lat->sleep_time);
        seq_printf(m, "Kernel stack trace:\n");
        __seqprint_stack_trace(m, high_lat->s_t);
        seq_printf(m, "User stack trace:\n");
        __seqprint_stack_trace(m, high_lat->s_t_user);
        seq_printf(m, "\n");
    }
}

/* free the hashtable for a given pid */
void free_table(struct taskNode *tn) {
    int bkt;
    struct st_slot *temp_slot;
    struct hlist_node *temp;
    write_lock(&hash_lock);
    hash_for_each_safe(tn->st_ht, bkt, temp, temp_slot, hash_node) {
        hash_del(&temp_slot->hash_node);
        kfree(temp_slot->s_t);
        kfree(temp_slot->s_t_user);
        kfree(temp_slot);
    }
    write_unlock(&hash_lock);
}
