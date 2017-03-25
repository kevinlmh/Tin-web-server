all:
	gcc tinywebserver.c helper.c -o tiny -lpthread
	gcc cgi-bin/adder.c -o cgi-bin/adder
