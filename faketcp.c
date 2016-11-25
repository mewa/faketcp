#include "faketcp.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>

queue* conn_q = NULL;

#define DEBUG

int ftcp_socket() {
  return socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

int ftcp_bind(int socket, sockaddr* addr, socklen_t addrlen) {
  int v = 1;
  setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int));
  return bind(socket, addr, addrlen);
}

int ftcp_listen(int socket) {
  queue* q = __ftcp_queue();

  // listening must be non-blocking
  int ret = fcntl(socket, F_SETFL, O_NONBLOCK);

  ftcp_sck_ctl* conn = malloc(sizeof(ftcp_sck_ctl));
  conn->socket = socket;
  conn->type = FTCP_SCK_LISTEN;
  conn->inc_q = queue_create();

  // push connection to connection queue
  queue_push(q, conn);

  pthread_create(&conn->tid, NULL, (void*(*)(void*)) __ftcp_listen, conn);

  return ret;
}

void* __ftcp_listen(ftcp_sck_ctl* sck_ctl) {
  socklen_t addrlen = sizeof(sockaddr_in);
  sockaddr_in addr;

  int ret = 1;

  while (ret > 0) {
    ftcp_conn_ctl* ctl = malloc(sizeof(ftcp_conn_ctl));
    memset(ctl, 0, sizeof(ftcp_conn_ctl));

    ret = recvfrom(sck_ctl->socket, &ctl->data, sizeof(ftcp_conn_ctl_data), 0, (sockaddr*) &ctl->addr, &addrlen);

    queue_push(sck_ctl->inc_q, ctl);

#ifdef DEBUG
    printf("<--[LISTEN]-- %s:%d connected to server\n", inet_ntoa(((sockaddr_in*) &addr)->sin_addr), ((sockaddr_in*) &addr)->sin_port);
#endif
  }
  return NULL;
}

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen) {
#ifdef DEBUG
  printf("Awaiting connection\n");
#endif

  // Receive SYN
  ftcp_conn_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_conn_ctl));
  int ret = recvfrom(socket, &ctl, sizeof(ftcp_conn_ctl_data), 0, addr, addrlen);

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

    ret = sendto(socket, &ctl, sizeof(ftcp_conn_ctl_data), 0, (sockaddr*) &addr, *addrlen);

#ifdef DEBUG
    printf("-->%s:%d SYN-ACK token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
    
    if (ret < 0)
      return ret;

    // Receive ACK
    ret = recvfrom(socket, &ctl, sizeof(ftcp_conn_ctl_data), 0, addr, addrlen);
    
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
  
  ftcp_conn_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_conn_ctl));

  host_seq[socket] = rand();
  
  ctl.flags |= FTCP_SYN;
  ctl.client = host_seq[socket];
  
  ret = write(socket, &ctl, sizeof(ftcp_conn_ctl_data));

#ifdef DEBUG
  printf("-->%s:%d SYN token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
  
  if (ret < 0)
    return ret;

  ret = read(socket, &ctl, sizeof(ftcp_conn_ctl_data));
  
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
  ftcp_conn_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_conn_ctl));
  if (ret < 0)
    return ret;
  ret = read(socket, &ctl, sizeof(ftcp_conn_ctl_data));
  return ret;
}

int ftcp_read(int socket, void* data, size_t len) {
  return 0;
}

queue* __ftcp_queue()  {
  if (!conn_q)
    conn_q = queue_create(NULL);
  return conn_q;
}
