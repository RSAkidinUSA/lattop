#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/stacktrace.h>

#define STACK_DEPTH 16
#define PRINT_PREF  KERN_INFO "[lattop]: "

/* struct for sharing latency data */
struct lat_data {
    pid_t pid;
    unsigned long long time;
    struct stack_trace *s_t;
};

int ins_probe(void);
void rm_probe(void);

/* rb tree functions */
int rb_init(void);
/* int set_asleep(pid_t pid, unsigned long long time); */
int set_asleep(struct lat_data *ld);
/* void set_awake(pid_t pid, unsigned long long time); */
void set_awake(struct lat_data *ld);
void print_rb(void);
void print_rb_proc(struct seq_file *m);
void rb_free(void);

/* proc functions */
int proc_open(void);
void proc_close(void);
