#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "faketcp.h"
#include <errno.h>

char buffer[12];

int main(int argc, char** argv) {
  buffer[11] = 0;

  char* addr = argv[1];
  short port = atoi(argv[2]);
  int sock = ftcp_socket(PF_INET);
  
  if (sock < 0) {
    perror("sock");
    exit(-1);
  }
  sockaddr_in src_addr, dest_addr;

  memset((void*) &src_addr, 0, sizeof(sockaddr_in));

  src_addr.sin_family = AF_INET;
  src_addr.sin_addr.s_addr = INADDR_ANY;
  src_addr.sin_port = 0;

  memset((void*) &dest_addr, 0, sizeof(sockaddr_in));

  dest_addr.sin_family = AF_INET;
  inet_aton(addr, &dest_addr.sin_addr);
  dest_addr.sin_port = htons(port);

  socklen_t addr_size = sizeof(sockaddr);
  //if (ftcp_bind(sock, (sockaddr*) &dest_addr, addr_size) < 0)
  //perror("ftcp bind");
  if (ftcp_connect(sock, (sockaddr*) &dest_addr, addr_size) < 0)
    perror("ftcp connect");
  else
    printf("connected\n");
  char i = 0;
  while (1) {
    sprintf(buffer, "hello %d", i++);
    if (ftcp_write(sock, buffer, strlen(buffer)) < 0) {
      perror("send");
    } else
      printf("sent\n");
    if (ftcp_read(sock, buffer, 11) > 0)
      printf("read %s\n", buffer);
    else
      perror("ftcp_read");
    sleep(1);
  }
}
