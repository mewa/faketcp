#include "faketcp.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv) {
  char buffer[12];
  buffer[11] = 0;
  
  short port = atoi(argv[1]);
  
  int socket = ftcp_socket(PF_INET);
  sockaddr_in addr;

  memset((void*) &addr, 0, sizeof(sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  
  socklen_t addrlen = sizeof(sockaddr);

  if (ftcp_bind(socket, (sockaddr*) &addr, addrlen) < 0)
    perror("bind");

  if (ftcp_accept(socket, (sockaddr*) &addr, &addrlen) < 0)
    perror("accept");
  else
    while (1) {
      if (ftcp_read(socket, buffer, 11) < 0)
	printf("read: %s\n", buffer);
    }
  return 0;
}
