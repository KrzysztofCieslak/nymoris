ENTRY(_start)

SECTIONS
{
    . = 0x400000;

    .text : ALIGN(0x1000)
    {
        *(.text)
    }

    .rodata : ALIGN(0x1000)
    {
        *(.rodata)
    }
}
