[org 0x7c00]
KERNEL_OFFSET equ 0x1000    ; Memory offset where we will load our kernel

mov [BOOT_DRIVE], dl        ; BIOS stores boot drive in DL, save it

mov bp, 0x9000              ; Set up stack safely away from code
mov sp, bp

call load_kernel            ; Load kernel from disk
call switch_to_pm           ; Switch to 32-bit Protected Mode
jmp $                       ; Hang if we return

%include "gdt.asm"          ; Include GDT descriptor table

[bits 16]
load_kernel:
    mov ah, 0x02            ; BIOS read sector function
    mov al, 40              ; Read 40 sectors (approx 20KB) to ensure full layout load
    mov ch, 0x00            ; Cylinder 0
    mov dh, 0x00            ; Head 0
    mov cl, 0x02            ; Start reading from sector 2 (Sector 1 is bootloader)
    mov dl, [BOOT_DRIVE]    ; Drive number
    mov bx, KERNEL_OFFSET   ; Destination pointer
    int 0x13
    jc disk_error
    ret

disk_error:
    jmp $

[bits 16]
switch_to_pm:
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load Global Descriptor Table
    mov eax, cr0
    or eax, 0x1             ; Set Protected Mode bit in CR0 register
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; Far jump to flush CPU pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG        ; Update segment registers to point to GDT data
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000        ; Update stack position for 32-bit space
    mov esp, ebp
    call BEGIN_PM           ; Call entry point for the kernel

[bits 32]
BEGIN_PM:
    call KERNEL_OFFSET      ; Jump directly to the loaded C kernel entry address
    jmp $

BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55

