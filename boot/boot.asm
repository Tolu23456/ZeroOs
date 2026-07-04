; ZeroOs Stage 1 Bootloader - MBR (512 bytes)
; Loaded by BIOS at 0x7C00
; Loads Stage 2 from disk and jumps to it

[BITS 16]
[ORG 0x7C00]

STAGE2_OFFSET    equ 0x7E00
STAGE2_SECTORS   equ 64
BOOT_DRIVE       equ 0x80

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov bp, sp

    mov [boot_drive], dl

    in al, 0x92
    or al, 2
    out 0x92, al

    mov si, msg_boot
    call print_string

    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, STAGE2_OFFSET
    int 0x13
    jc disk_error

    jmp 0x0000:STAGE2_OFFSET

disk_error:
    mov si, msg_disk_err
    call print_string
    jmp halt

halt:
    cli
    hlt
    jmp halt

print_string:
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

boot_drive: db 0
msg_boot:   db 'ZeroOs Bootloader v0.1', 13, 10, 0
msg_disk_err: db 'Disk read error!', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
