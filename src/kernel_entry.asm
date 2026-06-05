[bits 64]
; 64-bit kernel entry point
global _start

_start:
    ; Call main kernel function
    call main
    
    ; Hang if main returns
    hlt
    jmp $
