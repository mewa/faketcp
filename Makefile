all:
	gcc -g main.c faketcp.c -o ftcp

clean:
	rm ftcp *dSYM *~ *#

test:
	./ftcp 127.0.0.1 8888