#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>

/* Hashtable code */

struct st_slot {
    struct hlist_node hash_node;
    struct stack_trace *s_t;
    u32 hash;
    unsigned long long sleep_time;
};

/* hash a stack trace using jhash */
static u32 __hash_st (struct stack_trace *s_t) {
    return jhash((void *)s_t->entries, STACK_DEPTH * sizeof(unsigned long), JHASH_INITVAL);
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
    struct stack_trace *temp_st;
    st_hash = __hash_st(ld->s_t);
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
        temp_st = __init_st(ld->s_t);
        /* print_stack_trace(ld->s_t, 0); */
        /* print_stack_trace(temp_st, 0); */
        if (temp_st == NULL) {
            goto no_st_mem;
        }
        temp_slot->sleep_time = 0;
        temp_slot->s_t = temp_st;
        temp_slot->hash = st_hash;
        hash_add(tn->st_ht, &temp_slot->hash_node, temp_slot->hash);
    }
    temp_slot->sleep_time += (ld->time - tn->start_sleep);
    return;

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
        seq_printf(m, "%*c%ps\n", 1, ' ', (void *)trace->entries[i]);
    }
}

/* print the table of a given task */
int print_table(struct seq_file *m, struct taskNode *tn) {
    int bkt;
    struct st_slot *temp_slot;
    unsigned long long sum = 0;
    int counter = 0;
    hash_for_each(tn->st_ht, bkt, temp_slot, hash_node) {
        seq_printf(m, "Stack trace hash: %8x, Latency: %15llu\n", \
                temp_slot->hash, temp_slot->sleep_time);
        __seqprint_stack_trace(m, temp_slot->s_t);
        sum += temp_slot->sleep_time;
        counter++;
    }
    return counter;
}

/* free the hashtable for a given pid */
void free_table(struct taskNode *tn) {
    int bkt;
    struct st_slot *temp_slot;
    struct hlist_node *temp;
    hash_for_each_safe(tn->st_ht, bkt, temp, temp_slot, hash_node) {
        hash_del(&temp_slot->hash_node);
        kfree(temp_slot->s_t);
        kfree(temp_slot);
    }
}
