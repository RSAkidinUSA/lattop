#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>
#include <linux/uaccess.h>


/* Hashtable code */

struct st_slot {
    struct hlist_node hash_node;
    struct stack_trace *st;
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

#define __BUF_SIZE 1024
/* hash a stack trace using jhash */
static u32 __hash_st (struct stack_trace *st) {
    int buflen;
    char buf[__BUF_SIZE];
    buflen = __snprint_stack_trace(buf, __BUF_SIZE, st, 0);
    return jhash((void *)buf, buflen * sizeof(char), JHASH_INITVAL);
}


/* add a trace to the hash table or update it if it exists */
/* return true if it exists (free the mem) or false if not */
bool add_trace(struct lat_data *ld, struct taskNode *tn) {
    u32 st_hash;
    bool found = false;
    struct st_slot *temp_slot;
    st_hash = __hash_st(tn->last_trace);
    hash_for_each_possible(tn->st_ht, temp_slot, hash_node, st_hash) {
        /* stack trace is already in the table */ 
        if (st_hash == temp_slot->hash) {
            found = true;
            break;
        }
    }
    if (!found) {
        /* stack trace is not in the table  */
        temp_slot = kmalloc(sizeof(*temp_slot), GFP_ATOMIC);
        if (temp_slot == NULL) {
            goto no_slot_mem;
        }
        temp_slot->sleep_time = 0;
        temp_slot->st = tn->last_trace;
        temp_slot->hash = st_hash;
        hash_add(tn->st_ht, &temp_slot->hash_node, temp_slot->hash);
    }
    temp_slot->sleep_time += (ld->time - tn->start_sleep);
    return found;

no_slot_mem:
    printk(PRINT_PREF "unable to save stack trace info, no memory left\n");
    return true; 
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
    hash_for_each(tn->st_ht, bkt, temp_slot, hash_node) {
        if (high_lat == NULL) {
            high_lat = temp_slot;
        } else if (high_lat->sleep_time < temp_slot->sleep_time) {
            high_lat = temp_slot;
        }
    }
    if (high_lat != NULL) {
        seq_printf(m, "Max stack trace latency: %-15llu\n", \
                high_lat->sleep_time);
        seq_printf(m, "Stack trace:\n");
        __seqprint_stack_trace(m, high_lat->st);
        seq_printf(m, "\n");
    }
}

/* free the hashtable for a given pid */
void free_table(struct taskNode *tn) {
    int bkt;
    struct st_slot *temp_slot;
    struct hlist_node *temp;
    hash_for_each_safe(tn->st_ht, bkt, temp, temp_slot, hash_node) {
        hash_del(&temp_slot->hash_node);
        kfree(temp_slot->st);
        kfree(temp_slot);
    }
}
