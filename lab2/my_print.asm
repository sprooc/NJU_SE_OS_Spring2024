; %rdi: location of the string, %rsi
section .text
global print
print:
  mov rdx, rsi
  mov rsi, rdi  
  mov rdi, 1
  mov rax, 1
  syscall
  ret  


