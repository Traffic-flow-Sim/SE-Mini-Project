#include <stdio.h>
#include <stdlib.h>
#include <queue.h> 
/*
 * Basic Traffic Management System simulation.
 * One queue, one signal, deterministic vehicle arrivals.
 * Time is represented by logical simulation ticks.
 */

#define DEFAULT_TICKS       20
#define ARRIVAL_INTERVAL    2
#define GREEN_DURATION      3
#define YELLOW_DURATION     1
#define RED_DURATION        3

typedef enum {
    RED,
    GREEN,
    YELLOW
} SignalState;

typedef struct {
    unsigned int id;
    unsigned int arrival_tick;
} Vehicle;

typedef struct {
    Vehicle items[QUEUE_CAPACITY];
    int front;
    int size;
} VehicleQueue;

static void queue_init(VehicleQueue *queue)
{
    queue->front = 0;
    queue->size = 0;
}

static int queue_is_empty(const VehicleQueue *queue)
{
    return queue->size == 0;
}

static int queue_is_full(const VehicleQueue *queue)
{
    return queue->size == QUEUE_CAPACITY;
}

static int enqueue(VehicleQueue *queue, Vehicle vehicle)
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

static int dequeue(VehicleQueue *queue, Vehicle *vehicle)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    *vehicle = queue->items[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    queue->size--;
    return 1;
}

static const char *signal_name(SignalState state)
{
    switch (state) {
        case RED:    return "RED";
        case GREEN:  return "GREEN";
        case YELLOW: return "YELLOW";
        default:     return "UNKNOWN";
    }
}

/* The cycle is RED -> GREEN -> YELLOW -> RED. */
static SignalState signal_for_tick(unsigned int tick)
{
    unsigned int cycle_length =
        RED_DURATION + GREEN_DURATION + YELLOW_DURATION;
    unsigned int position = (tick - 1) % cycle_length;

    if (position < RED_DURATION) {
        return RED;
    }

    position -= RED_DURATION;
    if (position < GREEN_DURATION) {
        return GREEN;
    }

    return YELLOW;
}

int main(void)
{
    VehicleQueue queue;
    unsigned int next_vehicle_id = 1;
    unsigned int vehicles_passed = 0;
    unsigned int tick;
    unsigned int total_ticks = DEFAULT_TICKS;

    queue_init(&queue);

    printf("Traffic Management System - Basic Simulation\n");
    printf("Default settings: ticks=%u, queue capacity=%d, "
           "arrival interval=%d, signal cycle R/G/Y=%d/%d/%d ticks\n\n",
           total_ticks, QUEUE_CAPACITY, ARRIVAL_INTERVAL,
           RED_DURATION, GREEN_DURATION, YELLOW_DURATION);

    for (tick = 1; tick <= total_ticks; tick++) {
        SignalState signal = signal_for_tick(tick);
        Vehicle vehicle;

        printf("Tick %02u | Signal: %-6s | Queue before events: %d\n",
               tick, signal_name(signal), queue.size);

        /* A new vehicle arrives at a fixed interval. */
        if (tick == 1 || (tick - 1) % ARRIVAL_INTERVAL == 0) {
            Vehicle arriving_vehicle;
            arriving_vehicle.id = next_vehicle_id++;
            arriving_vehicle.arrival_tick = tick;

            if (enqueue(&queue, arriving_vehicle)) {
                printf("  Arrival: Vehicle %u (arrival tick %u) enqueued.\n",
                       arriving_vehicle.id, arriving_vehicle.arrival_tick);
            } else {
                printf("  Arrival: Vehicle %u rejected; queue is full.\n",
                       arriving_vehicle.id);
            }
        }

        /* Only green permits a vehicle to pass; one vehicle per tick. */
        if (signal == GREEN) {
            if (dequeue(&queue, &vehicle)) {
                vehicles_passed++;
                printf("  Passed:  Vehicle %u (arrived at tick %u).\n",
                       vehicle.id, vehicle.arrival_tick);
            } else {
                printf("  Movement: No vehicles waiting.\n");
            }
        } else {
            printf("  Movement: Vehicles wait while signal is %s.\n",
                   signal_name(signal));
        }

        printf("  Queue length at end of tick: %d\n\n", queue.size);
    }

    printf("Simulation complete after %u ticks.\n", total_ticks);
    printf("Vehicles passed: %u\n", vehicles_passed);
    printf("Vehicles remaining in queue: %d\n", queue.size);

    return EXIT_SUCCESS;
}
