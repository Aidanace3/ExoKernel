gdt_start:
gdt_null:                  ; Mandatory null descriptor
    dd 0x0
    dd 0x0
gdt_code:                  ; Code segment descriptor
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:                  ; Data segment descriptor
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

