#include "p03_module.h"
#include <linux/rbtree.h>

/* RB tree code */

struct intRoot {
    struct rb_root tree;
};

struct intNode {
    struct rb_node  int_node;
    int             int_key;
};

void init_intRoot(struct intRoot *intRoot) {
    intRoot->tree = RB_ROOT;
}

void __enqueue_intNode(struct intRoot *intRoot, struct intNode *in) {
    struct rb_node **link = &intRoot->tree.rb_node;
    struct rb_node *parent = NULL;
    struct intNode *entry;

    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct intNode, int_node);
        if (in->int_key < entry->int_key) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
        }
    }

    rb_link_node(&in->int_node, parent, link);
    rb_insert_color(&in->int_node, &intRoot->tree);
}

void rb_function(int *int_arr, int len) {
    struct intRoot *myRoot;
    struct intNode *currentNode, *lastNode;
    struct rb_node *tempNode, *rmNode;
    int i;
    
    /* create the RB tree */
    printk(PRINT_PREF "Creating the red-black tree\n");
    myRoot = kmalloc(sizeof(*myRoot), GFP_KERNEL);
    init_intRoot(myRoot);

    /* insert values into the rb tree */
    printk(PRINT_PREF "Inserting values into the red-black tree\n");
    for (i = 0; i < len; i++) {
        struct intNode *temp = kmalloc(sizeof(*temp), GFP_KERNEL);
        temp->int_key = int_arr[i];
        __enqueue_intNode(myRoot, temp);
    }

    /* iterate through rb tree and print values */
    printk(PRINT_PREF "red-black tree values:\n");
    tempNode = rb_last(&myRoot->tree);
    lastNode = rb_entry(tempNode, struct intNode, int_node);
    tempNode = rb_first(&myRoot->tree);
    currentNode = rb_entry(tempNode, struct intNode, int_node);

    while (currentNode != lastNode) {
        printk(PRINT_PREF "%d\n", currentNode->int_key);
        tempNode = rb_next(&currentNode->int_node);
        currentNode = rb_entry(tempNode, struct intNode, int_node);
    }
    /* print the last node in the tree */
    printk(PRINT_PREF "%d\n", currentNode->int_key);

    /* delete the rb tree */
    printk(PRINT_PREF "Removing items from the RB tree\n");
    tempNode = rb_first(&myRoot->tree);
    while (tempNode != rb_last(&myRoot->tree)) {
        rmNode = tempNode;
        tempNode = rb_next(tempNode);
        rb_erase(rmNode, &myRoot->tree);
        kfree(rb_entry(rmNode, struct intNode, int_node));
    }
    /* remove last item from the tree */
    rb_erase(tempNode, &myRoot->tree);
    kfree(rb_entry(tempNode, struct intNode, int_node));
    printk(PRINT_PREF "Done.\n");
}

