#ifndef ZEROOS_PROCESS_H
#define ZEROOS_PROCESS_H

#include <zeroos/types.h>
#include "idt.h"

#define MAX_PROCESSES 64
#define KERNEL_STACK_SIZE (PAGE_SIZE * 2)

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE
} process_state_t;

typedef struct process {
    uint64_t pid;
    process_state_t state;
    uint64_t *page_directory;      // PML4
    uint64_t rsp;                  // Stack pointer
    uint64_t rip;                  // Instruction pointer
    uint64_t kernel_stack;         // Kernel stack base
    uint64_t user_stack;           // User stack base
    char name[32];
    struct process *next;          // Linked list for scheduler
} process_t;

void process_init(void);
process_t *process_create(const char *name, uint64_t entry_point);
void process_destroy(process_t *proc);
void process_scheduler_tick(isr_regs_t *regs);
process_t *process_get_current(void);
uint64_t process_get_pid_count(void);

#endif
