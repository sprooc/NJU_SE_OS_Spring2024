IMG_FILE = ./lab2.img

compile:
	gcc -o main.o -c main.c -g 
	nasm -felf64 my_print.asm -o my_print.o -g
	gcc main.o my_print.o -o main

run: compile
	./main ${IMG_FILE}

debug: compile
	gdb ./main 

test:
	nasm -felf64 my_print.asm -o my_print.o -g
	gcc -c test.c -o test.o -g -no-pie
	gcc test.o my_print.o -o test -no-pie
	./test

clean:
	rm -f my_print.o main.o main test.o test
