[org 0x7c00]
KERNEL_OFFSET equ 0x1000    ; Memory offset where we will load our kernel

mov [BOOT_DRIVE], dl        ; BIOS stores boot drive in DL, save it

mov bp, 0x9000              ; Set up stack safely away from code
mov sp, bp

call load_kernel            ; Load kernel from disk
call enable_paging          ; Set up paging tables
call switch_to_long_mode    ; Switch to 64-bit Long Mode
jmp $                       ; Hang if we return

%include "gdt.asm"          ; Include GDT descriptor table

; Paging setup for 64-bit mode
; We'll use identity mapping (virtual addr = physical addr)
setup_page_tables:
    ; Create PML4 (Page Map Level 4) at 0x1000
    mov eax, 0x2000         ; PDP table address
    or eax, 0x3             ; Present + Writable
    mov dword [0x1000], eax
    mov dword [0x1004], 0x0 ; Upper 32 bits = 0

    ; Create PDP (Page Directory Pointer) at 0x2000
    mov eax, 0x3000         ; PD table address
    or eax, 0x3             ; Present + Writable
    mov dword [0x2000], eax
    mov dword [0x2004], 0x0

    ; Create PD (Page Directory) at 0x3000 with 2MB pages
    mov eax, 0x83           ; 2MB page, Present, Writable, PS (Page Size)
    mov ecx, 512            ; Fill all 512 entries
    xor ebx, ebx
.fill_pd:
    mov dword [0x3000 + ebx * 8], eax
    mov dword [0x3004 + ebx * 8], 0x0
    add eax, 0x200000       ; Next 2MB page
    inc ebx
    cmp ebx, ecx
    jl .fill_pd
    ret

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
enable_paging:
    call setup_page_tables
    
    ; Load CR3 with PML4 table address
    mov eax, 0x1000
    mov cr3, eax
    
    ; Enable PAE (Physical Address Extension) - required for 64-bit
    mov eax, cr4
    or eax, 0x20            ; Set PAE bit (bit 5)
    mov cr4, eax
    
    ret

[bits 16]
switch_to_long_mode:
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load Global Descriptor Table
    
    ; Enable Long Mode by setting EFER MSR
    mov ecx, 0xc0000080     ; EFER MSR
    rdmsr                   ; Read MSR
    or eax, 0x100           ; Set LME (Long Mode Enable) bit
    wrmsr                   ; Write MSR back
    
    ; Enable paging to activate Long Mode
    mov eax, cr0
    or eax, 0x80000001      ; Set PE (Protected Mode) and PG (Paging) bits
    mov cr0, eax
    
    jmp CODE_SEG:init_long_mode  ; Far jump to flush CPU pipeline and enter 64-bit mode

[bits 64]
init_long_mode:
    mov ax, DATA_SEG        ; Update segment registers for 64-bit
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov rbp, 0x90000        ; Update stack position for 64-bit space
    mov rsp, rbp
    
    xor rax, rax
    call KERNEL_OFFSET      ; Jump to the loaded kernel entry
    jmp $

BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55
