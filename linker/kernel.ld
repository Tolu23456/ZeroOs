/* ZeroOs Kernel Linker Script */
/* Kernel is loaded at 1MB (0x100000) by bootloader */
/* Linked at higher-half: 0xFFFFFFFF80000000 */

ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    __kernel_start = .;

    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    __bss_start = .;

    .bss : ALIGN(4K)
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    __bss_end = .;

    __kernel_end = .;
}
