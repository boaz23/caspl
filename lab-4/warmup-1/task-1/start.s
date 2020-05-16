stdout EQU 1

SYS_WRITE EQU 4
SYS_OPEN  EQU 5
SYS_CLOSE EQU 6

O_WRONLY EQU 1
O_APPEND EQU 1024

section .rodata
    s_hello: db "Hello, Infected File", 10, 0
    s_hello_len EQU 21

section .text
global _start
global system_call

global code_start
global code_end
global infection
global infector
extern main
_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv 
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop
        
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller

code_start:

infection:
    push ebp
    mov ebp, esp
    pushad
    
    push dword s_hello_len
    push dword s_hello
    push dword stdout
    push dword SYS_WRITE
    call system_call
    add esp, 16

    popad
    pop ebp
    ret

infector:
    push ebp
    mov ebp, esp
    sub esp, 4
    pushad

    push dword 0
    push dword O_WRONLY | O_APPEND
    push dword [ebp+8]
    push dword SYS_OPEN
    call system_call
    add esp, 16
    mov [ebp-4], eax

    push dword code_end - code_start
    push dword code_start
    push dword [ebp-4]
    push dword SYS_WRITE
    call system_call
    add esp, 16

    push dword [ebp-4]
    push dword SYS_CLOSE
    call system_call
    add esp, 8

    popad
    add esp, 4
    pop ebp
    ret

code_end:
    nop