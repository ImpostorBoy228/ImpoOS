ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 1K;

    .multiboot_header :
    {
        KEEP(*(.multiboot_header))
    }

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
}
