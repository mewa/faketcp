#pragma once

#include <unistd.h>

int ftcp_socket(int domain);

int ftcp_write(int socket, void* data, size_t len);

int ftcp_read(int socket, void* data, size_t len);
