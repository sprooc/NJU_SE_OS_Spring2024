%macro pusha 0
  push rax
  push rbx
  push rcx
  push rdx
%endmacro
%macro popa 0
  pop rdx
  pop rcx
  pop rbx
  pop rax
%endmacro

section .data
nums: db "0123456789abcdef"
signs: db "box", 0xA
error_msg: db "ERROR!", 0xA

section .bss
instr: resb 40

section .text
global _start:
_start:
  mov eax, 3
  mov ebx, 0
  mov rcx, instr
  mov edx, 40
  int 0x80
  mov rbx, 0
  mov rax, 0
  mov rdx, 0
  mov rcx, instr ; pointer
  cmp byte[rcx], 113
  je quit
.loop1:
  mov rdi, rbx
  mov rsi, rax
  call mul_ten
  mov dl, byte[rcx]
  cmp dl, 48
  jb error_handle
  cmp dl, 57
  ja error_handle
  sub dl, 48
  mov rdi,rbx
  mov rsi, rax
  call huge_add
  add rcx, 1
  cmp byte[rcx], 0x20
  jne .loop1
  add rcx, 1
  cmp byte[rcx], 98 ; 'b'
  je .pb
  cmp byte[rcx], 104 ; 'h'
  je .ph
  cmp byte[rcx], 111 ; 'o'
  je .po
  jmp error_handle
.pb:
  mov rdi, rbx
  mov rsi, rax
  call print_binary
  jmp _start
.ph:
  mov rdi, rbx
  mov rsi, rax
  call print_hex
  jmp _start
.po:
  mov rdi, rbx
  mov rsi, rax
  call print_oct
  jmp _start

quit:
  mov rax, 1
  mov rbx, 0
  int 0x80

; args: rdi, rsi
print_binary:
  push rbp
  mov rbp, rsp
  pusha
  mov rbx, rdi
  mov rax, rsi
  mov rdi, nums
  call print_char
  mov rdi, signs
  call print_char
  mov rdx, 1
  shl rdx, 63
  mov rcx, 0   ; pre zeros
.loop2:
  cmp rdx, 0
  je break1
  mov rdi, nums
  test rbx, rdx
  je .pz1
  add rdi, 1
  mov rcx, 1
.pz1:
  shr rdx, 1
  cmp rcx, 0
  je .loop2
  call print_char
  jmp .loop2
break1:
  mov rdx, 1
  shl rdx, 63
.loop3:
  cmp rdx, 0
  je break2
  mov rdi, nums
  test rax, rdx
  je .pz2
  add rdi, 1
  mov rcx, 1
.pz2:
  shr rdx, 1
  cmp rcx, 0
  je .loop3
  call print_char
  jmp .loop3
break2:
  cmp rcx, 0
  jne .seg6
  mov edi, nums
  call print_char
.seg6:
  mov rdi, signs
  add rdi, 3
  call print_char
  popa
  leave
  ret

; args: rdi, rsi
print_hex:
  push rbp
  mov rbp, rsp
  pusha
  mov r8, rdi
  mov rdi, nums
  call print_char
  mov rdi, signs
  add rdi, 2
  call print_char
  mov ecx, 64
  mov rdx, 0x0f
  shl rdx, 60    ; mask
  mov bx, 0      ; pre zero
.loop4: 
  sub ecx, 4
  mov rax, r8
  and rax, rdx
  shr rax, cl
  mov rdi, nums
  add rdi, rax
  shr rdx, 4
  or bx, ax
  je .seg1
  mov bx, 1
  call print_char
.seg1:
  cmp ecx, 0
  jne .loop4
  mov ecx, 64
  mov rdx, 0x0f
  shl rdx, 60    ; mask
.loop5: 
  sub ecx, 4
  mov rax, rsi
  and rax, rdx
  shr rax, cl
  mov rdi, nums
  add rdi, rax 
  shr rdx, 4
  or bx, ax
  je .seg2
  mov bx, 1
  call print_char
.seg2:
  cmp ecx, 0
  jne .loop5
  cmp bx, 0
  jne .seg7
  mov edi, nums
  call print_char
.seg7:
  mov rdi, signs
  add rdi, 3
  call print_char
  popa
  leave
  ret


; args: rdi, rsi
print_oct:
  push rbp
  mov rbp, rsp
  pusha
  mov r8, rdi
  mov rdi, nums
  call print_char
  mov rdi, signs
  add rdi, 1
  call print_char
  mov ecx, 38
  mov rdx, 0x07
  shl rdx, 35    ; mask
  mov bx, 0      ; pre zero
.loop6: 
  sub ecx, 3
  mov rax, r8
  and rax, rdx
  shr rax, cl
  mov rdi, nums
  add rdi, rax
  shr rdx, 3
  or bx, ax
  je .seg3
  mov bx, 1
  call print_char
.seg3:
  cmp ecx, 2
  jne .loop6
  mov rax, r8
  and rax, 0x03
  shl rax, 1
  mov rdx, rsi
  shr rdx, 63
  or rax, rdx
  or bx, ax
  je .seg5
  mov rdi, nums
  add rdi, rax
  call print_char
.seg5:
  mov ecx, 63
  mov rdx, 0x07
  shl rdx, 60    ; mask
.loop7: 
  sub ecx, 3
  mov rax, rsi
  and rax, rdx
  shr rax, cl
  mov rdi, nums
  add rdi, rax 
  shr rdx, 3
  or bx, ax
  je .seg4
  mov bx, 1
  call print_char
.seg4:
  cmp ecx, 0
  jne .loop7
  cmp bx, 0
  jne .seg8
  mov edi, nums
  call print_char
.seg8:
  mov rdi, signs
  add rdi, 3
  call print_char
  popa
  leave
  ret

print_char:
  push rbp
  mov rbp, rsp
  pusha
  mov eax, 4
  mov ebx, 1
  mov rcx, rdi
  mov edx, 1
  int 0x80
  popa
  leave
  ret
  

; args:   rdi: high 64 bits, rsi: low 64 bits
; return: rbx: high 64 bits, rax: low 64 bits
mul_ten:
  push rbp 
  mov rbp, rsp
  push rdx
  push r8
  push r9
  mov rdx, 10
  mov rax, rsi
  mul rdx
  mov r8, rax
  mov r9, rdx
  mov rax, rdi
  mov rdx, 10
  mul rdx
  add r9, rax
  mov rax, r8
  mov rbx, r9
  pop r9
  pop r8
  pop rdx
  leave
  ret

; args:   rdi: high 64 bits, rsi: low 64 bits, rdx: add value
; return: rbx: high 64 bits, rax: low 64 bits
huge_add:
  mov rbx, rdi
  mov rax, rsi
  add rax, rdx
  jo overflow
  ret
overflow:
  add rbx, 1
  ret

error_handle:
  mov eax, 4
  mov ebx, 1
  mov rcx, error_msg
  mov edx, 7
  int 0x80
  mov eax, 1
  mov ebx, 0
  jmp _start