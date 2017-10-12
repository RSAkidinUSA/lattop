#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>

/* Hashtable code */

struct st_slot {
    struct hlist_node hash;
    struct stack_trace *s_t;
    int sleep_time;
};

/* hash a stack trace using jhash */
static u32 __hash_st (struct stack_trace *s_t) {
    return jhash((void *)s_t->entries, STACK_DEPTH, JHASH_INITVAL);
}

/* copy stacktrace data from the stack to heap */
static void __st_copy (struct stack_trace *src, struct stack_trace *dst) {
    int i = 0;
    dst->max_entries = STACK_DEPTH;
    dst->nr_entries = src->nr_entries;
    for (i = 0; i < STACK_DEPTH; i++) {
        dst->entries[i] = src->entries[i];
    }
}

/* add a trace to the hash table or update it if it exists */
void add_trace(struct stack_trace *s_t, struct taskNode *tn) {
    u32 st_hash, temp_hash;
    struct st_slot *temp_slot;
    struct stack_trace *temp_st;
    st_hash = __hash_st(s_t);
    hash_for_each_possible(tn->st_ht, temp_slot, hash, st_hash) {
        temp_st = temp_slot->s_t;
        temp_hash = __hash_st(temp_st);
        if (temp_hash == st_hash) {
           /* stack trace is already in the table */ 
        }
    }
    /* stack trace is not in the table  */
    temp_st = kmalloc(sizeof(*temp_st), GFP_ATOMIC);
    if (temp_st == NULL) {
        /* error */
    }
    __st_copy(s_t, temp_st);

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
