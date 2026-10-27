
#include <stdio.h>
#include <stdlib.h>

#define NUM_DIRECTIONS 4
#define MAX_VEHICLES 100

// Traffic directions
enum Direction {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST
};

// Each direction has its own independent queue
typedef struct {
    int vehicles[MAX_VEHICLES];
    int front;
    int rear;
    int count;
} Queue;

// Names used when displaying the directions
const char *directionNames[NUM_DIRECTIONS] = {
    "North", "South", "East", "West"
};

// Initialize an empty queue
void initializeQueue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

// Add a vehicle to a queue
int enqueue(Queue *q, int vehicleId) {
    if (q->count == MAX_VEHICLES) {
        return 0;
    }

    q->rear = (q->rear + 1) % MAX_VEHICLES;
    q->vehicles[q->rear] = vehicleId;
    q->count++;

    return 1;
}

// Remove one vehicle from a queue
int dequeue(Queue *q, int *vehicleId) {
    if (q->count == 0) {
        return 0;
    }

    *vehicleId = q->vehicles[q->front];
    q->front = (q->front + 1) % MAX_VEHICLES;
    q->count--;

    return 1;
}

// Display queue lengths and signal phases
void displayQueues(Queue queues[], int greenDirection) {
    int i;

    printf("\n========== TRAFFIC STATUS ==========\n");

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        printf("%-5s | Signal: %-5s | Queue length: %d\n",
               directionNames[i],
               (i == greenDirection) ? "GREEN" : "RED",
               queues[i].count);
    }

    printf("====================================\n");
}

// Allow vehicles to pass only from the green direction
void processGreenDirection(Queue queues[], int greenDirection) {
    int vehicleId;

    printf("\nGreen signal: %s\n",
           directionNames[greenDirection]);

    if (dequeue(&queues[greenDirection], &vehicleId)) {
        printf("Vehicle %d passed from %s.\n",
               vehicleId, directionNames[greenDirection]);
    } else {
        printf("No vehicles waiting in %s.\n",
               directionNames[greenDirection]);
    }
}



int main(void) {
    Queue queues[NUM_DIRECTIONS];
    int i;
    int nextVehicleId = 1;
    int vehicleCount[NUM_DIRECTIONS] = {3, 2, 4, 1};

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        initializeQueue(&queues[i]);
    }

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        int j;
        for (j = 0; j < vehicleCount[i]; j++) {
            enqueue(&queues[i], nextVehicleId++);
        }
    }

    printf("TRAFFIC MANAGEMENT SIMULATION\n");

    // Give each direction one green phase
    for (i = 0; i < NUM_DIRECTIONS; i++) {
        printf("\n--- SIGNAL PHASE %d ---\n", i + 1);
        displayQueues(queues, i);
        processGreenDirection(queues, i);
        displayQueues(queues, i);
    }

    return 0;
}
