IMG_FILE = ./a.img

compile:
	gcc -o main.o -c main.c -g 
	nasm -felf64 my_print.asm -o my_print.o -g
	ld -dynamic-linker /usr/lib64/ld-linux-x86-64.so.2 -lc main.o my_print.o  /lib/x86_64-linux-gnu/crt1.o -o main

run: compile
	./main ${IMG_FILE}

debug: compile
	gdb ./main 

test:
	nasm -felf64 io.asm -o io.o -g
	gcc -c test.c -o test.o -g
	ld -dynamic-linker /usr/lib64/ld-linux-x86-64.so.2 -lc test.o io.o /lib/x86_64-linux-gnu/crt1.o -o test
	./test

clean:
	rm -f my_print.o main.o main test.o test