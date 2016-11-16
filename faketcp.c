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

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen) {
  #ifdef DEBUG
  printf("Awaiting connection\n");
  #endif
  
  ftcp_ctl ctl;
  int ret = recvfrom(socket, &ctl, sizeof(ftcp_ctl), 0, addr, addrlen);

#ifdef DEBUG
  printf("%s connected to server\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr));
#endif
  
  if (ret < 0)
    return ret;

  /*
    Just SYN, SYN+ACK = refresh
   */
  if (ctl.flags & FTCP_SYN && !(ctl.flags & FTCP_ACK)) {
#ifdef DEBUG
    printf("SYN\n");
#endif
    
    ctl.flags = FTCP_ACK;
    ctl.token = time(0);

#ifdef DEBUG
    printf("ACK token %d\n", ctl.token);
#endif

    sockaddr_in svr_addr;
    socklen_t svr_addrlen;
    getsockname(socket, (sockaddr*) &svr_addr, &svr_addrlen);

    bind(socket, (sockaddr*) &svr_addr, svr_addrlen);
    
    ret = write(socket, &ctl, sizeof(ftcp_ctl));
    
    if (ret < 0)
      return ret;
  }
  return ret;
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

  read(socket, &ctl, sizeof(ftcp_ctl));

  if (ret < 0)
    return ret;
  
  fcntl(socket, F_SETFL, O_NONBLOCK);
}

int ftcp_write(int socket, void* data, size_t len) {
  return write(socket, data, len);
}

int ftcp_read(int socket, void* data, size_t len) {
  
}
