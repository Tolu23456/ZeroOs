; ZeroOs Stage 2 Bootloader
; Loaded at 0x7E00 by Stage 1
; Responsibilities:
;   1. Detect memory map (E820)
;   2. Load kernel to 1MB
;   3. Enter 64-bit long mode
;   4. Jump to kernel

[BITS 16]
[ORG 0x7E00]

KERNEL_OFFSET   equ 0x100000    ; 1MB - kernel load address
KERNEL_SECTORS  equ 100         ; Max sectors to load for kernel
BOOT_DRIVE       equ 0x80

stage2_start:
    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string_16

    ; ============================================
    ; Step 1: Detect memory map using E820
    ; ============================================
    call detect_memory

    ; ============================================
    ; Step 2: Load kernel from disk to 1MB
    ; ============================================
    mov si, msg_loading_kernel
    call print_string_16

    call load_kernel

    ; ============================================
    ; Step 3: Enter Protected Mode
    ; ============================================
    mov si, msg_enter_pm
    call print_string_16

    cli

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Enable PE bit in CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to flush prefetch and load CS with code segment
    jmp 0x08:protected_mode_entry

; ============================================
; 16-bit Functions
; ============================================

print_string_16:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

detect_memory:
    mov di, 0x8000          ; Store memory map at 0x8000
    xor ebx, ebx            ; Continuation value (0 for first call)
    xor bp, bp              ; Entry counter
    mov edx, 0x534D4150     ; 'SMAP' signature

.e820_loop:
    mov eax, 0xE820
    mov [di + 20], dword 1  ; Force valid entry
    mov ecx, 24             ; Entry size
    int 0x15
    jc .e820_done           ; CF set = error or done
    cmp eax, 0x534D4150    ; Check for 'SMAP'
    jne .e820_done

    inc bp
    add di, 24

    cmp ebx, 0              ; EBX=0 means last entry
    je .e820_done
    jmp .e820_loop

.e820_done:
    mov [memory_entries], bp
    ret

load_kernel:
    ; Use INT 13h extensions to load kernel
    ; First, reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; Load kernel sectors
    mov ah, 0x42            ; Extended read
    mov dl, [boot_drive]
    mov si, dap             ; Disk Address Packet
    int 0x13
    jc .kernel_read_err
    ret

.kernel_read_err:
    mov si, msg_kernel_err
    call print_string_16
    jmp halt_16

halt_16:
    cli
    hlt
    jmp halt_16

; ============================================
; Data
; ============================================

boot_drive:     db 0
memory_entries: dw 0

msg_stage2:         db 'ZeroOs Stage 2 loaded', 13, 10, 0
msg_loading_kernel: db 'Loading kernel...', 13, 10, 0
msg_enter_pm:       db 'Entering protected mode...', 13, 10, 0
msg_kernel_err:     db 'Kernel read error!', 13, 10, 0

; Disk Address Packet for extended read
dap:
    db 0x10                ; Size of DAP (16 bytes)
    db 0                   ; Reserved
    dw KERNEL_SECTORS      ; Number of sectors
    dw 0x0000              ; Offset
    dw 0x0010              ; Segment (0x0010:0x0000 = 0x10000 = 64KB... we want 1MB)
    dd 6                   ; Start LBA (sector 6, after MBR + Stage 2)
    dd 0                   ; Upper 32 bits of LBA

; ============================================
; GDT (Global Descriptor Table)
; ============================================

gdt_start:
    ; Null descriptor (required)
    dd 0x0
    dd 0x0

gdt_code:       ; Code segment: base=0, limit=0xFFFFF, 4KB granularity, 64-bit
    dw 0xFFFF   ; Limit (bits 0-15)
    dw 0x0000   ; Base (bits 0-15)
    db 0x00     ; Base (bits 16-23)
    db 10011010b ; Access: present, ring 0, code, readable
    db 11001111b ; Flags: 4KB granularity, 64-bit, limit bits 16-19
    db 0x00     ; Base (bits 24-31)

gdt_data:       ; Data segment: base=0, limit=0xFFFFF, 4KB granularity
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b ; Access: present, ring 0, data, writable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; Size of GDT - 1
    dd gdt_start                  ; Offset of GDT

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; No pad here - 32-bit and 64-bit code follows below
; Single pad at end of file ensures correct size

; ============================================
; 32-bit Protected Mode Code
; ============================================

[BITS 32]

protected_mode_entry:
    ; Set up segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; Stack at 576KB

    ; Set up identity-mapped page tables for kernel at 1MB
    ; We need to map:
    ;   Virtual 0x00000000 -> Physical 0x00000000 (first 4GB identity)
    ;   Virtual 0xFFFFFFFF80000000 -> Physical 0x00100000 (kernel high half)
    ;
    ; For simplicity in Stage 2, we identity-map the first 4GB
    ; and also map the kernel at its high-half address

    mov edi, 0x1000         ; PML4 at 4KB
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd               ; Clear page tables

    ; PML4[0] -> PDPT at 0x2000
    mov dword [0x1000], 0x2003  ; Present + Writable
    ; PML4[511] -> PDPT at 0x2000 (for high half: 0xFFFFFFFF80000000)
    mov dword [0x1000 + 511*8], 0x2003

    ; PDPT[0] -> PD at 0x3000
    mov dword [0x2000], 0x3003

    ; PD entries: map first 2MB using 2MB huge pages
    ; PD[0] = 0x00000083 (2MB, present, writable) -> physical 0
    mov dword [0x3000], 0x00000083
    ; PD[1] = 0x00200083 -> physical 2MB
    mov dword [0x3000 + 8], 0x00200083
    ; ... map enough for kernel (up to 8MB = 4 entries)
    mov dword [0x3000 + 16], 0x00400083
    mov dword [0x3000 + 24], 0x00600083

    ; Also map kernel virtual address (0xFFFFFFFF80000000 = PML4[511], PDPT[510], PD[0])
    ; PDPT[510] -> PD at 0x3000 (reuse same PD for identity + high-half)
    mov dword [0x2000 + 510*8], 0x3003

    ; Load PML4 into CR3
    mov edi, 0x1000
    mov cr3, edi

    ; Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set long mode bit in EFER MSR
    mov ecx, 0xC0000080     ; EFER MSR
    rdmsr
    or eax, 1 << 8          ; LME (Long Mode Enable)
    wrmsr

    ; Enable paging (set PG bit in CR0)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_descriptor]

    ; Jump to 64-bit code
    jmp 0x08:long_mode_entry

; ============================================
; 64-bit GDT
; ============================================

gdt64_start:
    dq 0x0                  ; Null descriptor

gdt64_code:
    dw 0xFFFF               ; Limit
    dw 0x0000               ; Base
    db 0x00                 ; Base
    db 10011010b            ; Access
    db 10101111b            ; Flags: 64-bit
    db 0x00                 ; Base

gdt64_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

CODE64 equ gdt64_code - gdt64_start
DATA64 equ gdt64_data - gdt64_start

; ============================================
; 64-bit Long Mode Code
; ============================================

[BITS 64]

long_mode_entry:
    ; Set up 64-bit segment registers
    mov ax, DATA64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    ; Store E820 memory map location for kernel
    mov rdi, 0x8000         ; RDI = memory map pointer (first argument)
    movzx rsi, word [memory_entries]  ; RSI = number of entries

    ; Jump to kernel at 1MB
    jmp KERNEL_OFFSET

; Halt on error
halt_32:
    cli
    hlt
    jmp halt_32

; Pad to ensure we fit in 64 sectors
times 512*64 - ($ - $$) db 0
