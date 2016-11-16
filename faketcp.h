#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

#define FTCP_SYN 0x1
#define FTCP_ACK 0x2

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct ftcp_ctl {
  uint8_t flags;
  uint32_t token;
} ftcp_ctl;

int ftcp_socket(int domain);

int ftcp_bind(int socket, sockaddr* addr, socklen_t addrlen);

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen);

int ftcp_connect(int socket, sockaddr* addr, socklen_t addrlen);

int ftcp_write(int socket, void* data, size_t len);

int ftcp_read(int socket, void* data, size_t len);
