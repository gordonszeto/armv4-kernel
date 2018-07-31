ENTRY (main)

MEMORY
{
    redboot :   ORIGIN = 0x00000000, LENGTH = 0x218000
    ram :       ORIGIN = 0x00218000, LENGTH = 0x1DC5000
    rom :       ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
    .text :
    {
        *(.text)
        *(.got)
        *(.got.plt)
        *(.rodata)
        *(.rodata.str1.4)
        *(.glue_7)
        *(.glue_7t)
        _etext = .;
    } >ram

    .data :
    {
        *(.data)
        _edata = .;
    } >ram

    .bss :
    {
        *(.bss)
        _ebss = .;
    } >ram

    .irq_stack :
    {
        . = ALIGN(4);

        . += 0x100;

        _irq_stack_start = .;
    } >ram


    _stack_start = ORIGIN(ram) + LENGTH(ram);
}
