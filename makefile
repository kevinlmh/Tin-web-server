all:
	gcc -c tinywebserver.c -pthread
	gcc -c helper.c
	gcc -c threadpool.c -pthread
	gcc tinywebserver.o helper.o threadpool.o -o tiny -pthread
	gcc cgi-bin/adder.c -o cgi-bin/adder
