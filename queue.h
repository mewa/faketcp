#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct node_t node_t;
typedef struct queue queue;

struct node_t {
  node_t* next;
  void* value;
};

typedef struct queue {
  node_t* head;
  node_t* tail;
  size_t size;
} queue;

queue* queue_create(void* head);

void queue_push(queue* queue, void* item);

void* queue_pop(queue* queue);

void queue_destroy(queue* queue);


