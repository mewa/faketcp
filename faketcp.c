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
#include <errno.h>

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
  queue* q = __ftcp_conn_queue();
  
  // listening must be non-blocking
  int ret = 0;// = fcntl(socket, F_SETFL, O_NONBLOCK)
  ftcp_sck_ctl* conn = malloc(sizeof(ftcp_sck_ctl));  
  conn->socket = socket;
  conn->type = FTCP_SCK_LISTEN;
  conn->inc_q = queue_create();

  // push connection to connection queue
  queue_push(q, conn);

#ifdef DEBUG
  printf("[LISTEN] Started listening on socket %d\n", socket);
#endif
  
  pthread_create(&conn->tid, NULL, (void*(*)(void*)) __ftcp_listen, conn);

  return ret;
}

void* __ftcp_listen(ftcp_sck_ctl* sck_ctl) {
  int ret = 0;
  int err = EAGAIN;
  ftcp_conn_ctl* ctl = malloc(sizeof(ftcp_conn_ctl));

  while (ret > 0 || err == EAGAIN || err == EWOULDBLOCK) {
    ctl->addrlen = sizeof(ctl->addr);
    
    ret = recvfrom(sck_ctl->socket, &ctl->data, sizeof(ftcp_conn_ctl_data), 0, (sockaddr*) &ctl->addr, &ctl->addrlen);
    err = errno;

    if (ret > 0) {
      queue_push(sck_ctl->inc_q, ctl);

#ifdef DEBUG
      printf("<--[LISTEN]-- %s:%d connected to server\n", inet_ntoa(ctl->addr.sin_addr), ntohs(ctl->addr.sin_port));
#endif
      
      ctl = malloc(sizeof(ftcp_conn_ctl));
      memset(ctl, 0, sizeof(ftcp_conn_ctl));
    }

    usleep(10000);
  }
  free(ctl);
  perror("__ftcp_listen");
  return NULL;
}

int ftcp_accept(int socket, sockaddr* addr, socklen_t* addrlen) {
#ifdef DEBUG
  printf("[ACCEPT] Awaiting connection, socket %d\n", socket);
#endif
  queue* sck_q = __ftcp_sck_queue(socket);
  if (!sck_q)
    return -0xb4d;
  
  // Receive SYN from queue
  ftcp_conn_ctl* ctl = queue_pop(sck_q);
  while (!ctl) {
    ctl = queue_pop(sck_q);
    sleep(1);
  }
  printf("<--[ACCEPT]-- %s:%d connected to server\n", inet_ntoa(ctl->addr.sin_addr), ntohs(ctl->addr.sin_port));

  int ret = ftcp_socket();

  int conn_sck = ret;

  if (ret < 0)
    goto done;
#ifdef DEBUG
  printf("<--[ACCEPT]-- %s:%d connected to server\n", inet_ntoa(ctl->addr.sin_addr), ntohs(ctl->addr.sin_port));
#endif
  
  
  if (ctl->data.flags & FTCP_SYN && !(ctl->data.flags & FTCP_ACK)) {
    // Save host sequence
    host_seq[socket] = rand();
    // Save client sequence
    client_seq[socket] = ctl->data.client;
    
#ifdef DEBUG
    printf("<--[ACCEPT]-- %s:%d SYN token %d\n", inet_ntoa(((sockaddr_in*) &ctl->addr)->sin_addr), ((sockaddr_in*) &ctl->addr)->sin_port, client_seq[socket]);
#endif
    
    // Send SYN-ACK
    
    ctl->data.flags |= FTCP_ACK;
    // Put host sequence
    ctl->data.host = host_seq[socket];
    ctl->data.client = ++client_seq[socket];
    
    ret = connect(conn_sck, (sockaddr*) &ctl->addr, ctl->addrlen);

    if (ret < 0)
      goto done;

    ret = sendto(conn_sck, &ctl->data, sizeof(ftcp_conn_ctl_data), 0, (sockaddr*) &ctl->addr, ctl->addrlen);

    if (ret < 0)
      goto done;
    
#ifdef DEBUG
    printf("-->%s:%d SYN-ACK token %d\n", inet_ntoa(((sockaddr_in*) &ctl->addr)->sin_addr), ((sockaddr_in*) &ctl->addr)->sin_port, host_seq[socket]);
#endif
    
    // Receive ACK
    ret = recvfrom(conn_sck, &ctl->data, sizeof(ftcp_conn_ctl_data), 0, (sockaddr*) &ctl->addr, &ctl->addrlen);
    
    if (ret < 0)
      goto done;

    if ((ctl->data.flags & (FTCP_ACK | FTCP_SYN)) == FTCP_ACK) {
      if (ctl->data.client == client_seq[socket] + 1) {
#ifdef DEBUG
	printf("%s:%d ACK token %d\n", inet_ntoa(((sockaddr_in*) &ctl->addr)->sin_addr), ((sockaddr_in*) &ctl->addr)->sin_port, client_seq[socket]);
#endif
	++client_seq[socket];
      }
    }
  }
 done:
  if (ret < 0) {
    perror("accept_ret");
    return ret;
  }
  return conn_sck;
}

int ftcp_connect(int socket, sockaddr* addr, socklen_t addrlen) {
  int ret = connect(socket, addr, addrlen);
  if (ret < 0)
    return ret;
  
  ftcp_conn_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_conn_ctl));

  host_seq[socket] = rand();
  
  ctl.data.flags |= FTCP_SYN;
  ctl.data.client = host_seq[socket];
  
  ret = write(socket, &ctl.data, sizeof(ftcp_conn_ctl_data));

#ifdef DEBUG
  printf("-->%s:%d SYN token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
  
  if (ret < 0)
    return ret;

  ret = read(socket, &ctl.data, sizeof(ftcp_conn_ctl_data));
  
  if (ret < 0)
    return ret;

#ifdef DEBUG
  printf("<--%s:%d SYN-ACK token %d\n", inet_ntoa(((sockaddr_in*) addr)->sin_addr), ((sockaddr_in*) addr)->sin_port, host_seq[socket]);
#endif
  
  ctl.data.flags = FTCP_ACK;
  ret = write(socket, &ctl.data, sizeof(ftcp_conn_ctl_data));  

#ifdef DEBUG
  printf("ACK %d\n", ctl.data.flags & FTCP_ACK);
#endif

  return ret;
}

int ftcp_write(int socket, void* data, size_t len) {
  int ret = write(socket, data, len);
  ftcp_conn_ctl ctl;
  memset(&ctl, 0, sizeof(ftcp_conn_ctl));
  if (ret < 0)
    return ret;
  ret = read(socket, &ctl.data, sizeof(ftcp_conn_ctl_data));
  return ret;
}

int ftcp_read(int socket, void* data, size_t len) {
  return 0;
}


queue* __ftcp_conn_queue()  {
  if (!conn_q)
    conn_q = queue_create();
  return conn_q;
}

queue* __ftcp_sck_queue(int socket)  {
  node_t* node = __ftcp_conn_queue()->head;
  if (node) {
    while (node != NULL && ((ftcp_sck_ctl*) node->value)->socket != socket) {
      node = node->next;
    }
    if (node)
      return ((ftcp_sck_ctl*) node->value)->inc_q;
  }
  return NULL;
}
