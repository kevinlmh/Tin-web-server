all:
	gcc -c tin.c -pthread
	gcc -c helper.c
	gcc -c threadpool.c -pthread
	gcc tin.o helper.o threadpool.o -o tin -pthread
	gcc cgi-bin/adder.c -o cgi-bin/adder
