#include "process.h"
#include "mm.h"
#include "vmm.h"
#include "heap.h"
#include "string.h"
#include "vga.h"
#include "idt.h"

static process_t processes[MAX_PROCESSES];
static process_t *current_process = NULL;
static process_t *process_list = NULL;
static uint64_t next_pid = 1;

// Shared kernel page tables (set up by stage2)
extern uint64_t *_kernel_pml4;

void process_init(void) {
    memset(processes, 0, sizeof(processes));

    // Create idle process (PID 0)
    processes[0].pid = 0;
    processes[0].state = PROC_RUNNING;
    processes[0].page_directory = NULL;  // Uses kernel page tables
    processes[0].kernel_stack = 0;
    processes[0].user_stack = 0;
    strcpy(processes[0].name, "idle");
    processes[0].next = NULL;

    current_process = &processes[0];
    process_list = &processes[0];

    // Register scheduler on timer interrupt
    register_interrupt_handler(32, process_scheduler_tick);

    vga_printf("PROC: Initialized, PID 0 = idle\n");
}

process_t *process_create(const char *name, uint64_t entry_point) {
    // Find free slot
    process_t *proc = NULL;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            proc = &processes[i];
            break;
        }
    }
    if (!proc) return NULL;

    // Allocate kernel stack
    void *kstack = pmm_alloc_page();
    if (!kstack) return NULL;
    memset(kstack, 0, PAGE_SIZE);

    // Allocate user stack
    void *ustack = pmm_alloc_page();
    if (!ustack) {
        pmm_free_page(kstack);
        return NULL;
    }
    memset(ustack, 0, PAGE_SIZE);

    // Set up process
    proc->pid = next_pid++;
    proc->state = PROC_READY;
    proc->kernel_stack = (uint64_t)kstack + PAGE_SIZE;
    proc->user_stack = (uint64_t)ustack + PAGE_SIZE;
    proc->rip = entry_point;
    proc->rsp = proc->kernel_stack;
    strcpy(proc->name, name);
    proc->page_directory = NULL;  // Share kernel page tables for now

    // Add to process list
    proc->next = process_list->next;
    process_list->next = proc;

    vga_printf("PROC: Created '%s' (PID %d)\n", name, proc->pid);
    return proc;
}

void process_destroy(process_t *proc) {
    if (!proc || proc->pid == 0) return;

    proc->state = PROC_UNUSED;

    // Free stacks
    if (proc->kernel_stack) {
        pmm_free_page((void*)(proc->kernel_stack - PAGE_SIZE));
    }
    if (proc->user_stack) {
        pmm_free_page((void*)(proc->user_stack - PAGE_SIZE));
    }

    vga_printf("PROC: Destroyed PID %d\n", proc->pid);
}

void process_scheduler_tick(isr_regs_t *regs) {
    // Simple round-robin scheduler
    if (!current_process) return;

    // Save current process state
    current_process->rsp = regs->rsp;
    current_process->rip = regs->rip;

    // Find next ready process
    process_t *next = current_process->next;
    if (!next) next = process_list;

    while (next != current_process) {
        if (next->state == PROC_READY) {
            break;
        }
        next = next->next;
        if (!next) next = process_list;
    }

    if (next->state == PROC_READY && next != current_process) {
        current_process->state = PROC_READY;
        next->state = PROC_RUNNING;
        current_process = next;

        // Update registers for context switch
        regs->rsp = next->rsp;
        regs->rip = next->rip;
    }
}

process_t *process_get_current(void) {
    return current_process;
}

uint64_t process_get_pid_count(void) {
    return next_pid;
}
