
#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY 8

typedef struct {
    unsigned int id;
    unsigned int arrival_tick;
} Vehicle;

typedef struct {
    Vehicle items[QUEUE_CAPACITY];
    int front;
    int size;
} VehicleQueue;

void queue_init(VehicleQueue *queue);
int queue_is_empty(const VehicleQueue *queue);
int queue_is_full(const VehicleQueue *queue);
int enqueue(VehicleQueue *queue, Vehicle vehicle);
int dequeue(VehicleQueue *queue, Vehicle *vehicle);

#endif
