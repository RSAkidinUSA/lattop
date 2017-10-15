#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>

/* Hashtable code */

struct st_slot {
    struct hlist_node hash_node;
    struct stack_trace *s_t;
    u32 hash;
    int sleep_time;
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
        return temp;
    }
    temp->max_entries = STACK_DEPTH;
    temp->nr_entries = src->nr_entries;
    for (i = 0; i < STACK_DEPTH; i++) {
        temp->entries[i] = src->entries[i];
    }
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
            /* update sleep time for this task */
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
        if (temp_st == NULL) {
            goto no_st_mem;
        }
        temp_slot->sleep_time = 0;
        temp_slot->s_t = temp_st;
        temp_slot->hash = st_hash;
        hash_add(tn->st_ht, &temp_slot->hash_node, temp_slot->hash);
    }
    temp_slot->sleep_time += (ld->time - tn->start_sleep);

no_st_mem:
    kfree(temp_slot);
no_slot_mem:
    printk(PRINT_PREF "unable to save stack trace info, no memory left\n");
    return; 


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
/*

int hash_function(int *int_arr, int len) {
    int i, bkt;
    struct int_slot *int_slot;
    struct hlist_node *temp;
    
    // create the hashtable
    printk(PRINT_PREF "Creating the hashtable.\n");
    hash_init(int_slots_hash);

    // add values to the hash table
    printk(PRINT_PREF "Adding values to the hashtable.\n");
    for (i = 0; i < len; i++) {
        struct int_slot *temp = kmalloc(sizeof(*temp), GFP_KERNEL);
        if (temp == NULL) {
            return -ENOMEM;
        }
        temp->val = int_arr[i];
        hash_add(int_slots_hash, &temp->hash, temp->val);
    }

    // print all values in the hashtable
    printk(PRINT_PREF "Iterating the hashtable:\n");
    hash_for_each(int_slots_hash, bkt, int_slot, hash) {
        printk(PRINT_PREF "%d\n", int_slot->val);
    }
    PRINT_DONE;

    // Look up each number in hash table
    printk(PRINT_PREF "Looking up inserted numbers:\n");
    for (i = 0; i < len; i++) {
        hash_for_each_possible(int_slots_hash, int_slot, hash, int_arr[i]) {
            if(int_arr[i] == int_slot->val) {
                printk(PRINT_PREF "Input:%d, Hashed:%d\n", int_arr[i], \
                        int_slot->val);
            }
        }
    }
    PRINT_DONE;

    // delete all items in the hashtable
    printk(PRINT_PREF "Deleting the objects in the hashtable.\n");
    hash_for_each_safe(int_slots_hash, bkt, temp, int_slot, hash) {
        hash_del(&int_slot->hash);
        kfree(int_slot);
    }

    // iterate the empty has table
    // same code as before, but hashtable should be empty now
    printk(PRINT_PREF "Iterating the hashtable:\n");
    hash_for_each(int_slots_hash, bkt, int_slot, hash) {
        printk(PRINT_PREF "%d\n", int_slot->val);
    }
    PRINT_DONE;

    return 0;
}

*/
