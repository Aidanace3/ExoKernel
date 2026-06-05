; 64-bit GDT for Long Mode
gdt_start:
gdt_null:                  ; Mandatory null descriptor
    dd 0x0
    dd 0x0
gdt_code:                  ; 64-bit Code segment descriptor
    dw 0xffff, 0x0
    db 0x0, 10011010b, 10101111b, 0x0  ; L bit set (bit 5 = 1) for 64-bit
gdt_data:                  ; Data segment descriptor
    dw 0xffff, 0x0
    db 0x0, 10010010b, 10001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dq gdt_start            ; Changed from dd to dq for 64-bit address

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
