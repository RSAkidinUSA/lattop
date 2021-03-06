#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/stacktrace.h>
#include <linux/hashtable.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#define STACK_DEPTH 16
#define ST_HASH_BITS 4
#define PRINT_PREF  KERN_INFO "[lattop]: "

/* struct for sharing latency data */
struct lat_data {
    pid_t pid;
    char name[TASK_COMM_LEN];
    unsigned long long time;
    struct stack_trace *st;
};

struct taskNode {
    struct rb_node  lat_node, pid_node;
    long long       sleep_time; /* key */
    int             offset; /* offset for key matches */
    pid_t           pid;
    char            name[TASK_COMM_LEN];
    struct stack_trace *last_trace; /* stack trace before going to sleep */
    long long       start_sleep; /* when the task started sleeping, -1 if it isn't asleep */
    /* hashtable of stack traces for this pid */
    DECLARE_HASHTABLE(st_ht, ST_HASH_BITS);
};


int ins_probe(void);
void rm_probe(void);

/* rb tree functions */
int rb_init(void);
int set_asleep(struct lat_data *ld);
void set_awake(struct lat_data *ld);
void print_rb_proc(struct seq_file *m);
void rb_free(void);

/* hash functions */
bool add_trace(struct lat_data *ld, struct taskNode *tn);
void free_table(struct taskNode *tn);
void print_table(struct seq_file *m, struct taskNode *tn);

/* proc functions */
int proc_open(void);
void proc_close(void);

/* stack trace function */
void save_stack_trace_user(struct stack_trace *trace);
