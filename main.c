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
  struct sockaddr_in src_addr, dest_addr;

  memset((void*) &src_addr, 0, sizeof(struct sockaddr_in));

  src_addr.sin_family = AF_INET;
  src_addr.sin_addr.s_addr = INADDR_ANY;
  src_addr.sin_port = 0;

  memset((void*) &dest_addr, 0, sizeof(struct sockaddr_in));

  dest_addr.sin_family = AF_INET;
  inet_aton(addr, &dest_addr.sin_addr);
  dest_addr.sin_port = htons(port);

  socklen_t addr_size = sizeof(struct sockaddr);
  fcntl(sock, F_SETFL, O_NONBLOCK);
  connect(sock, (struct sockaddr*) &dest_addr, addr_size);
  while (1) {
    if (ftcp_write(sock, "hello", 6) < 0) {
      printf("econnreset %d\n", errno == ECONNREFUSED);
      perror("send");
    } else
      printf("sent\n");
    sleep(1);
  }
}
