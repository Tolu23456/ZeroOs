; ZeroOs Kernel Entry Point
; Called from Stage 2 bootloader (64-bit long mode)
; RDI = pointer to E820 memory map
; RSI = number of E820 entries

[BITS 64]

global _start
extern kmain

section .text

_start:
    ; Set up kernel stack
    mov rsp, 0x90000
    mov rbp, rsp

    ; Clear BSS section
    extern __bss_start
    extern __bss_end
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    shr rcx, 3              ; Divide by 8 for qword clearing
    xor rax, rax
    rep stosq

    ; RDI and RSI already set by Stage 2
    ; RDI = E820 map pointer, RSI = entry count
    call kmain

    ; If kmain returns, halt
    cli
.hang:
    hlt
    jmp .hang
