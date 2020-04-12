section	.rodata			; we define (global) read-only variables in .rodata section
	format_string: db "%s", 10, 0	; format string

section .bss			; we define (global) uninitialized variables in .bss section
	an: resb 12		; enough to store integer in [-2,147,483,648 (-2^31) : 2,147,483,647 (2^31-1)]

section .text
	global convertor
	extern printf

convertor:
	push ebp
	mov ebp, esp	
	pushad			

	mov ecx, dword [ebp+8]	; get function argument (pointer to string)

	; your code comes here...
	; int convertor(char* buf)
	
	; make space for local vars
	sub esp, 16
	; len = [0] = len(buf) - 1
	; n = [1] = parseInt(buf)
	; hex_len = [2] = len(reverseConvertToHex(n))

	; len = len(buf) - 1
	mov eax, 0
	buf_len_loop: ; while (!(buf[eax] == '\n' || buf[eax] == '\0'))
		; condition check
		; if (buf[eax] == '\n' || buf[eax] == '\0') break;
		mov ebx, [ebp + 8]
		add ebx, eax
		cmp byte [ebx], 10
		jz buf_len_loop_end
		cmp byte [ebx], 0
		jz buf_len_loop_end

		; increment
		; eax++
		inc dword eax
		jmp buf_len_loop
	buf_len_loop_end:
	dec dword eax
	mov dword [esp + 16], eax

	mov dword ecx, [esp + 16] ; i = len
	mov dword [esp + 12], 0 ; n = 0
	mov dword ebx, 1 ; pow = 1
	convert_to_num_loop: ; while (len >= 0)
		; condition check
		cmp ecx, 0
		jl convert_to_num_loop_end

		; body

		; digit = (buf[i] - 48) * pow
		mov eax, [ebp + 8] ; eax = buf
		movzx eax, byte [eax + ecx] ; eax = buf[i]
		sub eax, 48 ; eax -= 48;
		mul dword ebx ; eax = eax * pow

		; n += digit
		add dword [esp + 12], eax

		; loop increment
		; pow *= 10
		mov eax, ebx
		mov edx, 10
		mul edx
		mov ebx, eax

		; i--
		dec dword ecx
		jmp convert_to_num_loop
	convert_to_num_loop_end:

	mov eax, [esp + 12] ; n
	mov ecx, 0 ; hex_len = 0

	cmp eax, 0
	jnz num_to_hex_string_reversed_loop

	num_to_hex_string_reversed_zero:
	mov byte [an], 48
	inc ecx
	jmp num_to_hex_string_reversed_loop_end

	num_to_hex_string_reversed_loop: ; while (n > 0)
		; condition check
		cmp eax, 0
		jz num_to_hex_string_reversed_loop_end

		;body

		; digit = n % 16
		mov ebx, eax
		and bl, 0xF

		; if (digit < 10)
		cmp bl, 10
		jge append_hex_letter_digit
		append_decimal_digit:
			add bl, 48
			jmp append_digit_end
		append_hex_letter_digit:
			add bl, 55
		append_digit_end:

		mov byte [an + ecx], bl

		; loop increment
		shr eax, 4 ; n /= 16
		inc ecx ; hex_len++
		jmp num_to_hex_string_reversed_loop
	num_to_hex_string_reversed_loop_end:
	mov byte [an + ecx], 0
	mov dword [esp + 8], ecx

	; reverse the string in an
	mov ebx, 0 ; i = 0
	; k = hex_len - 1
	mov eax, [esp + 8]
	dec eax
	reverse_hex_string_loop: ; while (i < k)
		; condition check
		; if (eax < ebx) break;
		cmp eax, ebx
		jl reverse_hex_string_loop_end

		; body
		; swap(&an[i], &an[k])
		mov byte dl, [an + ebx]
		mov byte cl, [an + eax]
		mov byte [an + ebx], cl
		mov byte [an + eax], dl

		; loop increment
		inc ebx
		dec eax
		jmp reverse_hex_string_loop
	reverse_hex_string_loop_end:

	add esp, 16
	mov ecx, [ebp + 8]
	; end our code

	push an			; call printf with 2 arguments -  
	push format_string	; pointer to str and pointer to format string
	call printf
	add esp, 8		; clean up stack after call

	popad			
	mov esp, ebp	
	pop ebp
	ret
