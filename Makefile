all:
	gcc -g main.c faketcp.c -o ftcp

server:
	gcc -g server.c faketcp.c -o fsvr

clean:
	rm ftcp *dSYM *~ *#

test:
	./ftcp 127.0.0.1 8888

stest:
	./fsvr 127.0.0.1 8888
