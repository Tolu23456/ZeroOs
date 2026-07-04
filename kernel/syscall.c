#include "syscall.h"
#include "process.h"
#include "vga.h"
#include "timer.h"
#include "io.h"

typedef syscall_result_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t);

static syscall_fn_t syscall_table[SYS_MAX];

// Syscall implementations
static syscall_result_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    process_t *proc = process_get_current();
    if (proc) {
        vga_printf("PROC: PID %d exit with code %d\n", proc->pid, code);
        process_destroy(proc);
    }
    return (syscall_result_t){0, ERR_NONE};
}

static syscall_result_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
    (void)fd;
    const char *str = (const char*)buf;
    for (size_t i = 0; i < len; i++) {
        vga_putchar(str[i]);
    }
    return (syscall_result_t){len, ERR_NONE};
}

static syscall_result_t sys_read(uint64_t fd, uint64_t buf, uint64_t len) {
    (void)fd; (void)buf; (void)len;
    return (syscall_result_t){-1, ERR_NOT_FOUND};
}

static syscall_result_t sys_fork(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    return (syscall_result_t){-1, ERR_NOT_FOUND};
}

static syscall_result_t sys_exec(uint64_t path, uint64_t a2, uint64_t a3) {
    (void)path; (void)a2; (void)a3;
    return (syscall_result_t){-1, ERR_NOT_FOUND};
}

static syscall_result_t sys_sbrk(uint64_t delta, uint64_t a2, uint64_t a3) {
    (void)delta; (void)a2; (void)a3;
    return (syscall_result_t){-1, ERR_NOT_FOUND};
}

static syscall_result_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    process_t *proc = process_get_current();
    if (proc) {
        return (syscall_result_t){proc->pid, ERR_NONE};
    }
    return (syscall_result_t){0, ERR_NONE};
}

static syscall_result_t sys_sleep(uint64_t ms, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    timer_sleep(ms);
    return (syscall_result_t){0, ERR_NONE};
}

void syscall_init(void) {
    syscall_table[SYS_EXIT]   = sys_exit;
    syscall_table[SYS_WRITE]  = sys_write;
    syscall_table[SYS_READ]   = sys_read;
    syscall_table[SYS_FORK]   = sys_fork;
    syscall_table[SYS_EXEC]   = sys_exec;
    syscall_table[SYS_SBRK]   = sys_sbrk;
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_SLEEP]  = sys_sleep;

    vga_printf("SYSCALL: %d syscalls registered\n", SYS_MAX);
}

syscall_result_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (num >= SYS_MAX || !syscall_table[num]) {
        return (syscall_result_t){-1, ERR_INVALID_ARG};
    }
    return syscall_table[num](a1, a2, a3);
}
