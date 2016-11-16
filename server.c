#include "faketcp.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv) {
  char* addr = argv[1];
  short port = atoi(argv[2]);
  
  int socket = ftcp_socket(PF_INET);
  sockaddr_in src_addr, dest_addr;

  memset((void*) &src_addr, 0, sizeof(sockaddr_in));
  src_addr.sin_family = AF_INET;
  inet_aton(addr, &src_addr.sin_addr);
  src_addr.sin_port = htons(port);
  
  socklen_t addrlen = sizeof(sockaddr);

  if (bind(socket, (sockaddr*) &src_addr, addrlen) < 0)
    perror("bind");
  
  if (ftcp_accept(socket, (sockaddr*) &dest_addr, &addrlen) < 0)
    perror("accept");
  
  return 0;
}
