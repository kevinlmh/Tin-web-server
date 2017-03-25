all:
	gcc tinywebserver.c helper.c -o tiny
	gcc cgi-bin/adder.c -o cgi-bin/adder
