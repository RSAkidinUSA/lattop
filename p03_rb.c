#include "p03_module.h"
#include <linux/rbtree.h>
#include <linux/types.h>

/* tree usage is as follows: */
/* init the tree */
/* check if item is in tree */
/* 1) if yes, save info and remove */
/* create new node with updated info */
/* add to tree */
/* delete tree when finished */

/* RB tree code */

struct taskRoot {
    struct rb_root tree;
};

/* root of the tree */
static struct taskRoot *myRoot;

struct taskNode {
    struct rb_node  task_node;
    long long       sleep_time; /* key */
    pid_t           pid;
    long long       start_sleep; /* when the task started sleeping, -1 if it isn't asleep */
};

void init_taskRoot(struct taskRoot *taskRoot) {
    taskRoot->tree = RB_ROOT;
}

/* add a node to the tree */
static void __add_node(struct taskNode *tn) {
    struct rb_node **link = &myRoot->tree.rb_node;
    struct rb_node *parent = NULL;
    struct taskNode *entry;

    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct taskNode, task_node);
        if (tn->sleep_time < entry->sleep_time) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
        }
    }

    rb_link_node(&tn->task_node, parent, link);
    rb_insert_color(&tn->task_node, &myRoot->tree);
}

/* print the tree */
void printTree(void) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;

    tempNode = rb_first(&myRoot->tree);

    while (tempNode != NULL) {
        currentNode = rb_entry(tempNode, struct taskNode, task_node);
        printk(PRINT_PREF "PID: %u, Sleep time: %llu", currentNode->pid, \
                currentNode->sleep_time);
        tempNode = rb_next(&currentNode->task_node);
    }
}

/* search tree for a node with a given PID */
static struct taskNode *__searchRB(pid_t pid) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;

    tempNode = rb_first(&myRoot->tree);

    while (tempNode != NULL) {
        currentNode = rb_entry(tempNode, struct taskNode, task_node);
        if (currentNode->pid == pid) {
            return currentNode;
        }
        tempNode = rb_next(&currentNode->task_node);
    }
    return NULL;
}

/* set asleep */
/* check if task in tree, if so, set sleep time */
/* else create new node and add */
/* if unable to allocate memory return an error */
int set_asleep(pid_t pid, long long time) {
    struct taskNode *temp;
    temp = __searchRB(pid);
    if (temp == NULL) {
        temp = kmalloc(sizeof(*temp), GFP_KERNEL);
        if (temp == NULL) {
            return ENOMEM;
        }
        temp->pid = pid;
        temp->sleep_time = 0;
    } else {
        rb_erase(&temp->task_node, &myRoot->tree);
    }
    temp->start_sleep = time;
    __add_node(temp);
    return 0;
}

/* set awake */
/* check if task in tree, if so, add diff to sleep time */
/* if not, return -1 */
int set_awake(pid_t pid, long long time) {
    
    return 0;
}



/* initialize the rb tree */
int rb_init(void) {
    /* create the RB tree */
    myRoot = kmalloc(sizeof(*myRoot), GFP_KERNEL);
    init_taskRoot(myRoot);
    if (myRoot == NULL) {
        return ENOMEM;
    }
    return 0;  
}

/* delete the rb tree when done */
void rb_free(void) {
    struct rb_node *rmNode, *tempNode;
    
    tempNode = rb_first(&myRoot->tree);
    while (tempNode != NULL) {
        rmNode = tempNode;
        tempNode = rb_next(tempNode);
        rb_erase(rmNode, &myRoot->tree);
        kfree(rb_entry(rmNode, struct taskNode, task_node));
    }
    /* free the root */
    kfree(myRoot);
}

