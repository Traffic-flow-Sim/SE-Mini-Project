
#include "queue.h"

void queue_init(VehicleQueue *queue)
{
    queue->front = 0;
    queue->size = 0;
}

int queue_is_empty(const VehicleQueue *queue)
{
    return queue->size == 0;
}

int queue_is_full(const VehicleQueue *queue)
{
    return queue->size == QUEUE_CAPACITY;
}

int enqueue(VehicleQueue *queue, Vehicle vehicle)
{
    int rear;

    if (queue_is_full(queue)) {
        return 0;
    }

    rear = (queue->front + queue->size) % QUEUE_CAPACITY;
    queue->items[rear] = vehicle;
    queue->size++;

    return 1;
}

int dequeue(VehicleQueue *queue, Vehicle *vehicle)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    *vehicle = queue->items[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    queue->size--;

    return 1;
}
