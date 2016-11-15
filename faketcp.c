#include "faketcp.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>



int ftcp_socket(int domain) {
  int sock = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
  fcntl(sock, F_SETFL, O_NONBLOCK);
  return sock;
}

int ftcp_write(int socket, void* data, size_t len) {
  return write(socket, data, len);
}

int ftcp_read(int socket, void* data, size_t len) {
  
}
