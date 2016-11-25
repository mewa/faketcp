all: client server

client:
	gcc -g -Wall -pthread queue.c client.c faketcp.c -o ftcp

server:
	gcc -g -Wall -pthread queue.c server.c faketcp.c -o fsvr

clean:
	rm -rf ftcp fsvr *.dSYM *~ *#

ctest:
	./ftcp 127.0.0.1 8888

stest:
	./fsvr 8888
