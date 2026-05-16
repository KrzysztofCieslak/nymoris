ENTRY(_start)

PHDRS
{
    limine  PT_LOAD;
    text    PT_LOAD;
    rodata  PT_LOAD;
    data    PT_LOAD;
}

SECTIONS
{
    . = 0xffffffff80000000;

    .limine_requests : AT(ADDR(.limine_requests) - 0xffffffff80000000 + 0x100000) {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests.*))
        KEEP(*(.limine_requests_end))
    } :limine

    . = ALIGN(4K);

    .text : AT(ADDR(.text) - 0xffffffff80000000 + 0x100000) {
        *(.text .text.*)
    } :text

    . = ALIGN(4K);

    .rodata : AT(ADDR(.rodata) - 0xffffffff80000000 + 0x100000) {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4K);

    .data : AT(ADDR(.data) - 0xffffffff80000000 + 0x100000) {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data
}
