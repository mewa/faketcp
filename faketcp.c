#include "faketcp.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define DEBUG

int ftcp_socket(int domain) {
  return socket(domain, SOCK_DGRAM, IPPROTO_UDP);
}

int ftcp_bind(int socket, sockaddr* addr, socklen_t addrlen) {
  int v = 1;
  setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int));
#ifdef DEBUG
  printf("Socket %d bound\n", socket);
#endif
  return bind(socket, addr, addrlen);
}

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen) {
#ifdef DEBUG
  printf("Awaiting connection\n");
#endif

  // Wait for incoming connection
  ftcp_ctl ctl;
  int ret = recvfrom(socket, &ctl, sizeof(ftcp_ctl), 0, addr, addrlen);

#ifdef DEBUG
  printf("%s:%d connected to server\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port);
#endif
  
  if (ret < 0)
    return ret;

  // Just SYN, SYN+ACK = refresh
  if (ctl.flags & FTCP_SYN && !(ctl.flags & FTCP_ACK)) {
#ifdef DEBUG
    printf("SYN\n");
#endif
    // ACK request and set new token
    ctl.flags = FTCP_ACK;
    ctl.token = time(0);

#ifdef DEBUG
    printf("ACK token %d\n", ctl.token);
#endif
    
    ret = connect(socket, (sockaddr*) addr, *addrlen);
    
    if (ret < 0)
      return ret;

    //ret = sendto(socket, &ctl, sizeof(ftcp_ctl), 0, (sockaddr*) &svr_addr, svr_addrlen);
    ret = write(socket, &ctl, sizeof(ftcp_ctl));
    
    if (ret < 0)
      return ret;
  }
  return 0;
}

int ftcp_connect(int socket, sockaddr* addr, socklen_t addrlen) {
  int ret = connect(socket, addr, addrlen);
  if (ret < 0)
    return ret;
  
  ftcp_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_ctl));
  ctl.flags |= FTCP_SYN;
  ret = write(socket, &ctl, sizeof(ftcp_ctl));
  
  if (ret < 0)
    return ret;

  ret = read(socket, &ctl, sizeof(ftcp_ctl));
#ifdef DEBUG
  printf("ACK %d token %d\n", (ctl.flags & FTCP_ACK) != 0, ctl.token);
#endif
  //fcntl(socket, F_SETFL, O_NONBLOCK);

  return ret;
}

int ftcp_write(int socket, void* data, size_t len) {
  ftcp_ctl ctl;
  int ret = write(socket, data, len);
  return ret;
}

int ftcp_read(int socket, void* data, size_t len) {
  ftcp_ctl ctl;
  int ret = read(socket, &ctl, sizeof(ftcp_ctl));
  if (ret < 0)
    return ret;
#ifdef DEBUG
  printf("<--%d-- %s %s\n", ctl.token, ctl.flags & FTCP_SYN ? "SYN" : "", ctl.flags & FTCP_ACK ? "ACK" : "");
#endif
  return read(socket, data, len);
}
