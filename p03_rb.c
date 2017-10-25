#include "p03_module.h"
#include <linux/rbtree.h>
#include <linux/types.h>

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
static struct taskRoot *latRoot, *pidRoot;

/* local functions */

static void __init_taskRoot(struct taskRoot *taskRoot) {
    taskRoot->tree = RB_ROOT;
}

/* add a node to the lat tree return false if unable to insert */

static bool __add_node_l(struct taskNode *tn) {
    struct rb_node **link = &latRoot->tree.rb_node;
    struct rb_node *parent = NULL;
    struct taskNode *entry;
    bool valid = true;
    /* add node to the latency rb tree */ 
    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct taskNode, lat_node);
        if (tn->sleep_time < entry->sleep_time) {
            link = &parent->rb_left;
        } else if (tn->sleep_time > entry->sleep_time) {
            link = &parent->rb_right;
        } else {
            /* if sleep times are equal, use offsets */
            if (tn->offset < entry->offset) {
                link = &parent->rb_left;
            } else if (tn->offset > entry->offset) {
                link = &parent->rb_right;
            } else {
                tn->offset++;
                valid = false; 
            }
        }
    }
    if (valid) {
        rb_link_node(&tn->lat_node, parent, link);
        rb_insert_color(&tn->lat_node, &latRoot->tree);
    }

    return valid;
}

/* add a node to the pid rb tree */

static void __add_node_p(struct taskNode *tn)  {
    struct rb_node **link = &pidRoot->tree.rb_node;
    struct rb_node *parent = NULL;
    struct taskNode *entry;
    /* add node to the latency rb tree */ 
    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct taskNode, pid_node);
        if (tn->pid < entry->pid) {
            link = &parent->rb_left;
        } else if (tn->pid > entry->pid) {
            link = &parent->rb_right;
        } else {
            /* error */
            printk(PRINT_PREF "Error inserting task with the same pid: %d\n", tn->pid);
            return;
        }
    }

    rb_link_node(&tn->pid_node, parent, link);
    rb_insert_color(&tn->pid_node, &pidRoot->tree);

}

/* search tree for a node with a given PID */
static struct taskNode *__searchRB(pid_t pid) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;

    tempNode = pidRoot->tree.rb_node;
    while (tempNode != NULL) {
        currentNode = rb_entry(tempNode, struct taskNode, pid_node);
        if (pid < currentNode->pid) {
            tempNode = tempNode->rb_left;
        } else if (pid > currentNode->pid) {
            tempNode = tempNode->rb_right;
        } else {
            break;
        }
    }
    return (tempNode == NULL) ? NULL: currentNode;
}

/* functions accessible by other files */

/* initialize the rb tree */
int rb_init(void) {
    /* create the latency RB tree */
    latRoot = kmalloc(sizeof(*latRoot), GFP_KERNEL);
    if (latRoot == NULL) {
        goto lat_err;
    }
    /* create the pid RB tree */
    pidRoot = kmalloc(sizeof(*latRoot), GFP_KERNEL);
    if (pidRoot == NULL) {
        goto pid_err;
    }

    
    __init_taskRoot(latRoot);
    __init_taskRoot(pidRoot);
    return 0;  

pid_err:
    kfree(latRoot);
lat_err:
    return ENOMEM;
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

/* set asleep */
/* check if task in tree, if so, set sleep time */
/* else create new node and add */
/* if unable to allocate memory return an error */
int set_asleep(struct lat_data *ld) {
    struct taskNode *temp;
    bool placed;
    write_lock(&rb_lock);
    temp = __searchRB(ld->pid);
    if (temp == NULL) {
        temp = kmalloc(sizeof(*temp), GFP_ATOMIC);
        if (temp == NULL) {
            /* should i really do this?? It'll crash the kernel */
            write_unlock(&rb_lock);
            return ENOMEM;
        }
        hash_init(temp->st_ht);
        temp->pid = ld->pid;
        temp->sleep_time = 0;

        __add_node_p(temp);
    } else {
        rb_erase(&temp->lat_node, &latRoot->tree);
    }
    /* this will prevent any issues with renaming of tasks */
    strncpy(temp->name, ld->name, TASK_COMM_LEN);
    temp->start_sleep = ld->time;
    temp->last_trace = __init_st(ld->st);
    temp->offset = 0;
    do {
        placed = __add_node_l(temp);
    } while (!placed);
    write_unlock(&rb_lock);

    return 0;
}

/* set awake */
/* if task not in tree or not sleeping, return */
/* else set the new sleep time */
void set_awake(struct lat_data *ld) {
    struct taskNode *temp;
    bool placed;
    write_lock(&rb_lock);
    temp = __searchRB(ld->pid);
    if (temp == NULL) {
        write_unlock(&rb_lock);
        return;
    } else if (temp->start_sleep  == -1) {
        write_unlock(&rb_lock);
        return;
    } else {
        rb_erase(&temp->lat_node, &latRoot->tree);
        temp->sleep_time += (ld->time - temp->start_sleep);
        /* if the trace is no longer needed, delete it */
        if (add_trace(ld, temp)) {
            kfree(temp->last_trace);
        }
        temp->start_sleep = -1;
        temp->offset = 0;
        do {
            placed = __add_node_l(temp);
        } while (!placed);
    }
    write_unlock(&rb_lock);
}

/* print the 1000 longest sleeping processes to /proc */
void print_rb_proc(struct seq_file *m) {
    struct rb_node *tempNode;
    struct taskNode *currentNode;
    int i = 0;
    unsigned long flags;

    tempNode = rb_last(&latRoot->tree);

    seq_printf(m, "Top 1000 highest latency processes:\n");
    read_lock_irqsave(&rb_lock, flags);
    while (tempNode != NULL && i < 1000) {
        currentNode = rb_entry(tempNode, struct taskNode, lat_node);
        if (currentNode->sleep_time == 0) {
            /* once a sleep time of zero is reached, exit */
            break;
        }
        seq_printf(m, "PID: %u, comm: %s, Latency: %-15llu\n", \
                currentNode->pid, currentNode->name, \
                currentNode->sleep_time);
        print_table(m, currentNode);
        tempNode = rb_prev(&currentNode->lat_node);
    }
    read_unlock_irqrestore(&rb_lock, flags);

}


/* delete the rb tree when done */
void rb_free(void) {
    struct rb_node *tempNode;
    struct taskNode *tempTask;
    write_lock(&rb_lock);
    
    /* free pid rb tree */
    tempNode = rb_first(&pidRoot->tree);
    while (tempNode != NULL) {
        rb_erase(tempNode, &pidRoot->tree);
        tempNode = rb_first(&pidRoot->tree);
    }

    /* free lat rb tree */ 
    tempNode = rb_first(&latRoot->tree);
    while (tempNode != NULL) {
        rb_erase(tempNode, &latRoot->tree);
        tempTask = rb_entry(tempNode, struct taskNode, lat_node);
        free_table(tempTask);
        kfree(tempTask);
        tempNode = rb_first(&latRoot->tree);
    }
    
    write_unlock(&rb_lock);
    /* free the root */
    kfree(latRoot);
    kfree(pidRoot);
}

