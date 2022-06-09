all: myELF

myELF: task3.c
	gcc -g -m32 -fno-pie -c -o task3.o task3.c
	gcc -g -m32 -fno-pie  task3.o -o myELF

1: task1.c
	gcc -g -m32 -fno-pie -c -o task1.o task1.c
	gcc -g -m32 -fno-pie  task1.o -o myELF

2: task2.c
	gcc -g -m32 -fno-pie -c -o task2.o task2.c
	gcc -g -m32 -fno-pie  task2.o -o myELF


.PHONY: clean
clean:
	rm -rf ./*.o myELF

