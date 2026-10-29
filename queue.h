
#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY 100

typedef struct {
    unsigned int id;
    unsigned int arrival_tick;
} Vehicle;

typedef struct {
    Vehicle items[QUEUE_CAPACITY];
    int front;
    int size;
    unsigned int dropped; /* vehicles rejected because the queue was full */
} VehicleQueue;

/*
 * Bounded FIFO queue.
 *
 * Overflow policy: when the queue is full, the arriving vehicle is dropped
 * (the queue is left unchanged) and counted in queue->dropped.
 * A NULL queue behaves as both empty and full, so no call can crash on it.
 */
void queue_init(VehicleQueue *queue);
int queue_is_empty(const VehicleQueue *queue);
int queue_is_full(const VehicleQueue *queue);

/* Returns 1 if stored, 0 if dropped (queue full or NULL). */
int enqueue(VehicleQueue *queue, Vehicle vehicle);

/* Returns 1 and fills *vehicle, or 0 if empty/NULL (*vehicle is untouched). */
int dequeue(VehicleQueue *queue, Vehicle *vehicle);

#endif
