#include "faketcp.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

int ftcp_socket(int domain) {
  return socket(domain, SOCK_DGRAM, IPPROTO_UDP);
}

int ftcp_bind(int socket, sockaddr* addr, socklen_t addrlen) {
  int v = 1;
  setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int));
  return bind(socket, addr, addrlen);
}

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen) {
#ifdef DEBUG
  printf("Awaiting connection\n");
#endif

  // Receive SYN
  ftcp_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_ctl));
  int ret = recvfrom(socket, &ctl, sizeof(ftcp_ctl), 0, addr, addrlen);

#ifdef DEBUG
  printf("<--%s:%d connected to server\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port);
#endif
  
  if (ret < 0)
    return ret;
  
  if (ctl.flags & FTCP_SYN && !(ctl.flags & FTCP_ACK)) {
    // Save host sequence
    host_seq[socket] = rand();
    // Save client sequence
    client_seq[socket] = ctl.client;
    
#ifdef DEBUG
    printf("<--%s:%d SYN token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, client_seq[socket]);
#endif

    // Send SYN-ACK
    
    ctl.flags |= FTCP_ACK;
    // Put host sequence
    ctl.host = host_seq[socket];
    ctl.client = ++client_seq[socket];
    
    ret = connect(socket, (sockaddr*) addr, *addrlen);
    
    if (ret < 0)
      return ret;

    ret = sendto(socket, &ctl, sizeof(ftcp_ctl), 0, (sockaddr*) &addr, *addrlen);

#ifdef DEBUG
    printf("-->%s:%d SYN-ACK token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
    
    if (ret < 0)
      return ret;

    // Receive ACK
    ret = recvfrom(socket, &ctl, sizeof(ftcp_ctl), 0, addr, addrlen);
    
    if (ret < 0)
      return ret;

    if ((ctl.flags & (FTCP_ACK | FTCP_SYN)) == FTCP_ACK) {
      if (ctl.client == client_seq[socket] + 1) {
#ifdef DEBUG
	printf("%s:%d ACK token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, client_seq[socket]);
#endif
	++client_seq[socket];
      }
    }
  }
  return 0;
}

int ftcp_connect(int socket, sockaddr* addr, socklen_t addrlen) {
  int ret = connect(socket, addr, addrlen);
  if (ret < 0)
    return ret;
  
  ftcp_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_ctl));

  host_seq[socket] = rand();
  
  ctl.flags |= FTCP_SYN;
  ctl.client = host_seq[socket];
  
  ret = write(socket, &ctl, sizeof(ftcp_ctl));

#ifdef DEBUG
  printf("-->%s:%d SYN token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
  
  if (ret < 0)
    return ret;

  ret = read(socket, &ctl, sizeof(ftcp_ctl));
  
  if (ret < 0)
    return ret;

#ifdef DEBUG
  printf("<--%s:%d SYN-ACK token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
  
#ifdef DEBUG
  printf("ACK %d\n", ctl.flags & FTCP_ACK);
#endif

  return ret;
}

int ftcp_write(int socket, void* data, size_t len) {
  int ret = write(socket, data, len);
  ftcp_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_ctl));
  if (ret < 0)
    return ret;
  ret = read(socket, &ctl, sizeof(ftcp_ctl));
  return ret;
}

int ftcp_read(int socket, void* data, size_t len) {
  return 0;
}
