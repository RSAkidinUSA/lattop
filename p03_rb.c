#include "p03_module.h"
#include <linux/rbtree.h>
#include <linux/types.h>
#include <linux/spinlock.h>

DEFINE_RWLOCK(rb_lock);

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

/* local functions */

static void __init_taskRoot(struct taskRoot *taskRoot) {
    taskRoot->tree = RB_ROOT;
}

/* add a node to the tree */
static void __add_node(struct taskNode *tn) {
    struct rb_node **link = &myRoot->tree.rb_node;
    struct rb_node *parent = NULL;
    struct taskNode *entry;
    
    write_lock(&rb_lock);
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
    write_unlock(&rb_lock);
}

/* search tree for a node with a given PID */
static struct taskNode *__searchRB(pid_t pid) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;

    tempNode = rb_first(&myRoot->tree);
    read_lock(&rb_lock);
    while (tempNode != NULL) {
        currentNode = rb_entry(tempNode, struct taskNode, task_node);
        if (currentNode->pid == pid) {
            break;
        }
        tempNode = rb_next(&currentNode->task_node);
    }
    read_unlock(&rb_lock);
    return (tempNode == NULL) ? NULL : currentNode;
}

/* functions accessible by other files */

/* initialize the rb tree */
int rb_init(void) {
    /* create the RB tree */
    myRoot = kmalloc(sizeof(*myRoot), GFP_KERNEL);
    if (myRoot == NULL) {
        return ENOMEM;
    }
    __init_taskRoot(myRoot);
    return 0;  
}

/* set asleep */
/* check if task in tree, if so, set sleep time */
/* else create new node and add */
/* if unable to allocate memory return an error */
int set_asleep(struct lat_data *ld) {
    struct taskNode *temp;
    temp = __searchRB(ld->pid);
    if (temp == NULL) {
        temp = kmalloc(sizeof(*temp), GFP_ATOMIC);
        if (temp == NULL) {
            /* should i really do this?? It'll crash the kernel */
            return ENOMEM;
        }
        hash_init(temp->st_ht);
        strncpy(temp->name, ld->name, TASK_COMM_LEN);
        temp->pid = ld->pid;
        temp->sleep_time = 0;
    } else {
        write_lock(&rb_lock);
        rb_erase(&temp->task_node, &myRoot->tree);
        write_unlock(&rb_lock);
    }
    temp->start_sleep = ld->time;
    __add_node(temp);
    return 0;
}

/* set awake */
/* if task not in tree or not sleeping, return */
/* else set the new sleep time */
void set_awake(struct lat_data *ld) {
    struct taskNode *temp;
    temp = __searchRB(ld->pid);
    if (temp == NULL) {
        return;
    } else if (temp->start_sleep  == -1) {
        return;
    } else {
        write_lock(&rb_lock);
        rb_erase(&temp->task_node, &myRoot->tree);
        write_unlock(&rb_lock);
        temp->sleep_time += (ld->time - temp->start_sleep);
        add_trace(ld, temp);
        temp->start_sleep = -1;
        __add_node(temp);
    }
}

/* print the 1000 longest sleeping processes to /proc */
void print_rb_proc(struct seq_file *m) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;
    int i = 0;
    unsigned long flags;

    tempNode = rb_last(&myRoot->tree);

    seq_printf(m, "Top 1000 highest latency processes:\n");
    read_lock_irqsave(&rb_lock, flags);
    while (tempNode != NULL && i < 1000) {
        currentNode = rb_entry(tempNode, struct taskNode, task_node);
        if (currentNode->sleep_time == 0) {
            /* once a sleep time of zero is reached, exit */
            break;
        }
        seq_printf(m, "PID: %u, comm: %s, Latency: %-15llu\n", \
                currentNode->pid, currentNode->name, \
                currentNode->sleep_time);
        print_table(m, currentNode);
        tempNode = rb_prev(&currentNode->task_node);
    }
    read_unlock_irqrestore(&rb_lock, flags);

}


/* delete the rb tree when done */
void rb_free(void) {
    struct rb_node *tempNode;
    struct taskNode *tempTask;
    write_lock(&rb_lock); 
    tempNode = rb_first(&myRoot->tree);
    while (tempNode != NULL) {
        rb_erase(tempNode, &myRoot->tree);
        tempTask = rb_entry(tempNode, struct taskNode, task_node);
        free_table(tempTask);
        kfree(tempTask);
        tempNode = rb_first(&myRoot->tree);
    }
    write_unlock(&rb_lock);
    /* free the root */
    kfree(myRoot);
}

