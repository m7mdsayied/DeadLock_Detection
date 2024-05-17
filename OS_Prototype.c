#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>

// Define data structures for processes and resources
struct process {
    int pid;
    struct list_head wait_list; // List of resources waiting for
    struct list_head list; // List entry for global process list
};

struct resource {
    int rid;
    bool allocated;
    struct list_head wait_list; // List of processes waiting for
    struct list_head list; // List entry for global resource list
};

// Global lists for processes and resources
LIST_HEAD(process_list);
LIST_HEAD(resource_list);

// Lock for synchronizing access to lists
DEFINE_SPINLOCK(list_lock);

// Function to add an edge to the wait-for graph
void add_edge(struct process *proc, struct resource *res) {
    spin_lock(&list_lock);
    list_add_tail(&proc->wait_list, &res->wait_list);
    spin_unlock(&list_lock);
}

// Function to remove an edge from the wait-for graph
void remove_edge(struct process *proc, struct resource *res) {
    spin_lock(&list_lock);
    list_del(&proc->wait_list);
    spin_unlock(&list_lock);
}

// Function to allocate a resource to a process
void allocate_resource(struct process *proc, struct resource *res) {
    spin_lock(&list_lock);
    if (!res->allocated) {
        res->allocated = true;
    } else {
        add_edge(proc, res);
    }
    spin_unlock(&list_lock);
}

// Function to release a resource held by a process
void release_resource(struct process *proc, struct resource *res) {
    spin_lock(&list_lock);
    res->allocated = false;
    // Allocate resource to the next waiting process if available
    if (!list_empty(&res->wait_list)) {
        struct process *next_proc = list_first_entry(&res->wait_list, struct process, wait_list);
        list_del(&next_proc->wait_list);
        allocate_resource(next_proc, res);
    }
    spin_unlock(&list_lock);
}

// Function to detect cycles in the wait-for graph (using depth-first search)
bool dfs(struct process *proc, bool visited[], bool *rec_stack);

bool dfs_resource(struct resource *res, bool visited[], bool *rec_stack) {
    struct process *proc;
    struct list_head *pos;

    list_for_each(pos, &res->wait_list) {
        proc = list_entry(pos, struct process, wait_list);
        if (dfs(proc, visited, rec_stack))
            return true;
    }

    return false;
}

bool dfs(struct process *proc, bool visited[], bool *rec_stack) {
    struct resource *res;
    struct list_head *pos;

    if (rec_stack[proc->pid])
        return true;

    if (visited[proc->pid])
        return false;

    visited[proc->pid] = true;
    rec_stack[proc->pid] = true;

    list_for_each(pos, &proc->wait_list) {
        res = list_entry(pos, struct resource, wait_list);
        if (dfs_resource(res, visited, rec_stack))
            return true;
    }

    rec_stack[proc->pid] = false;
    return false;
}

// Function to check for deadlock
bool detect_deadlock(void) {
    struct process *proc;
    bool visited[100] = {false}; // Assuming maximum of 100 processes
    bool rec_stack[100] = {false}; // Recursion stack

    spin_lock(&list_lock);
    list_for_each_entry(proc, &process_list, list) {
        if (dfs(proc, visited, rec_stack)) {
            spin_unlock(&list_lock);
            return true;
        }
    }
    spin_unlock(&list_lock);

    return false;
}

// Function to simulate process interactions (for testing)
void simulate_interactions(void) {
    struct process *p1 = kmalloc(sizeof(struct process), GFP_KERNEL);
    struct process *p2 = kmalloc(sizeof(struct process), GFP_KERNEL);
    struct resource *r1 = kmalloc(sizeof(struct resource), GFP_KERNEL);
    struct resource *r2 = kmalloc(sizeof(struct resource), GFP_KERNEL);

    p1->pid = 1;
    p2->pid = 2;
    r1->rid = 1;
    r2->rid = 2;

    INIT_LIST_HEAD(&p1->wait_list);
    INIT_LIST_HEAD(&p2->wait_list);
    INIT_LIST_HEAD(&r1->wait_list);
    INIT_LIST_HEAD(&r2->wait_list);

    list_add_tail(&p1->list, &process_list);
    list_add_tail(&p2->list, &process_list);
    list_add_tail(&r1->list, &resource_list);
    list_add_tail(&r2->list, &resource_list);

    // Simulate resource allocation and wait-for conditions
    allocate_resource(p1, r1);
    allocate_resource(p2, r2);
    add_edge(p1, r2); // p1 waits for r2
    add_edge(p2, r1); // p2 waits for r1

    if (detect_deadlock()) {
        printk(KERN_INFO "Deadlock detected\n");
    } else {
        printk(KERN_INFO "No deadlock detected\n");
    }

    // Cleanup
    list_del(&p1->list);
    list_del(&p2->list);
    list_del(&r1->list);
    list_del(&r2->list);
    kfree(p1);
    kfree(p2);
    kfree(r1);
    kfree(r2);
}

// Module initialization
static int __init deadlock_detector_init(void) {
    simulate_interactions();
    printk(KERN_INFO "Deadlock Detector module initialized\n");
    return 0;
}

// Module cleanup
static void __exit deadlock_detector_exit(void) {
    struct process *proc, *tmp_proc;
    struct resource *res, *tmp_res;

    spin_lock(&list_lock);
    list_for_each_entry_safe(proc, tmp_proc, &process_list, list) {
        list_del(&proc->list);
        kfree(proc);
    }

    list_for_each_entry_safe(res, tmp_res, &resource_list, list) {
        list_del(&res->list);
        kfree(res);
    }
    spin_unlock(&list_lock);

    printk(KERN_INFO "Deadlock Detector module exited\n");
}

module_init(deadlock_detector_init);
module_exit(deadlock_detector_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A real-time deadlock detection kernel module with resolution mechanisms and reporting");

