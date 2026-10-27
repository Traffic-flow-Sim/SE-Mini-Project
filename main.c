
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_DIRECTIONS 4
#define MAX_VEHICLES 100
#define MAX_DENSITY 100
#define MAX_SIGNAL_DURATION 120

// Read and validate a positive integer within the allowed range
int getValidatedInput(const char *prompt, int min, int max) {
    char input[100];
    char extra;
    int value;
    int result;

    while (1) {
        printf("%s", prompt);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error: Unable to read input.\n");
            exit(1);
        }

        result = sscanf(input, " %d %c", &value, &extra);

        if (result != 1) {
            printf("Error: Enter a valid whole number.\n");
            continue;
        }

        if (value < min || value > max) {
            printf("Error: Enter a value between %d and %d.\n",
                   min, max);
            continue;
        }

        return value;
    }
}

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

// Keep a green signal active for the configured duration
void runSignalPhase(int duration) {
    time_t startTime = time(NULL);

    printf("Green signal remains active for %d seconds.\n", duration);

    while (time(NULL) - startTime < duration) {
        // Wait until the configured duration has elapsed
    }

    printf("Green signal phase completed.\n");
}



int main(void) {
    Queue queues[NUM_DIRECTIONS];
    int i, j;
    int nextVehicleId = 1;
    int density;
    int signalDuration;

    printf("=== TRAFFIC MANAGEMENT CONFIGURATION ===\n");

    density = getValidatedInput(
        "Enter traffic density (1-100 vehicles per direction): ",
        1, MAX_DENSITY
    );

    signalDuration = getValidatedInput(
        "Enter green signal duration (1-120 seconds): ",
        1, MAX_SIGNAL_DURATION
    );

    printf("\nConfiguration saved for this run.\n");
    printf("Traffic density: %d vehicles per direction\n", density);
    printf("Green signal duration: %d seconds per direction\n",
           signalDuration);

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        initializeQueue(&queues[i]);
    }

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        for (j = 0; j < density; j++) {
            if (!enqueue(&queues[i], nextVehicleId++)) {
                printf("Error: Queue is full.\n");
                return 1;
            }
        }
    }

    printf("\nTRAFFIC MANAGEMENT SIMULATION\n");

        for (i = 0; i < NUM_DIRECTIONS; i++) {
        printf("\n--- SIGNAL PHASE %d (%d seconds) ---\n",
               i + 1, signalDuration);

        displayQueues(queues, i);

        runSignalPhase(signalDuration);

        processGreenDirection(queues, i);

        displayQueues(queues, i);
    }

    return 0;
}
