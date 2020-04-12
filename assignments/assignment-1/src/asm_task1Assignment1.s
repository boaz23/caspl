section .rodata
         format: db "%d" , 10, 0

section .text
    global assFunc
    extern c_checkValidity
    extern printf

assFunc:

    push ebp
    mov ebp, esp
    pushad

    push dword [ebp + 12]     ; y
    push dword [ebp + 8]      ; x

    call c_checkValidity 
    add esp, 8

    cmp eax, 0
    jz addtion
    ; eax = 1
    subtraction:
        mov edx, [ebp + 8]
        sub edx, [ebp + 12]
        jmp print_num
    ; eax = 0
    addtion:
        mov edx, [ebp + 8]
        add edx, [ebp + 12]

    print_num:
        push edx
        push dword format
        call printf
        add esp, 8
    
    popad
    pop ebp
    ret

    

