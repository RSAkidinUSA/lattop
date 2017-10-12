#include "p03_module.h"
#include <linux/hashtable.h>
#include <linux/jhash.h>

/* Hashtable code */

struct st_slot {
    struct hlist_node hash;
    struct stack_trace *s_t;
};

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
