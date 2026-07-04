#ifndef ZEROOS_SYSCALL_H
#define ZEROOS_SYSCALL_H

#include <zeroos/types.h>

// Syscall numbers
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_FORK    3
#define SYS_EXEC    4
#define SYS_SBRK    5
#define SYS_GETPID  6
#define SYS_SLEEP   7
#define SYS_MAX     8

// Syscall result structure (modern error handling)
typedef struct {
    int64_t value;
    uint32_t error;
} syscall_result_t;

// Error codes
enum {
    ERR_NONE = 0,
    ERR_INVALID_ARG,
    ERR_NO_MEMORY,
    ERR_NOT_FOUND,
    ERR_PERMISSION,
    ERR_IO,
};

void syscall_init(void);
syscall_result_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3);

#endif
