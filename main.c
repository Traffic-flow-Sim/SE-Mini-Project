
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "input.h"
#include "queue.h"

#define NUM_DIRECTIONS 4
#define MAX_SIGNAL_DURATION 120

enum Direction {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST
};

static const char *directionNames[NUM_DIRECTIONS] = {
    "North", "South", "East", "West"
};

static int getValidatedInput(const char *prompt, int min, int max)
{
    int value;

    /* Bounded, strict parsing lives in input.c so it can be unit tested. */
    if (prompt_int(stdin, stdout, prompt, min, max, &value) != 1) {
        fprintf(stderr, "Error: Unable to read input.\n");
        exit(EXIT_FAILURE);
    }

    return value;
}

static void displayQueues(
    const VehicleQueue queues[],
    int greenDirection)
{
    int i;

    printf("\n========== TRAFFIC STATUS ==========\n");

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        printf("%-5s | Signal: %-5s | Queue length: %d\n",
               directionNames[i],
               (i == greenDirection) ? "GREEN" : "RED",
               queues[i].size);
    }

    printf("====================================\n");
}

static void processGreenDirection(
    VehicleQueue queues[],
    int greenDirection)
{
    Vehicle vehicle;

    printf("\nGreen signal: %s\n",
           directionNames[greenDirection]);

    if (dequeue(&queues[greenDirection], &vehicle)) {
        printf("Vehicle %u passed from %s.\n",
               vehicle.id, directionNames[greenDirection]);
    } else {
        printf("No vehicles waiting in %s.\n",
               directionNames[greenDirection]);
    }
}

static void runSignalPhase(int duration)
{
    time_t startTime = time(NULL);

    printf("Green signal remains active for %d seconds.\n", duration);

    while (time(NULL) - startTime < duration) {
        /* Wait for the configured signal duration. */
    }

    printf("Green signal phase completed.\n");
}

int main(void)
{
    VehicleQueue queues[NUM_DIRECTIONS];
    unsigned int nextVehicleId = 1;
    int density;
    int signalDuration;
    int i;
    int j;

    printf("=== TRAFFIC MANAGEMENT CONFIGURATION ===\n");

    density = getValidatedInput(
        "Enter traffic density (1-100 vehicles per direction): ",
        1, QUEUE_CAPACITY);

    signalDuration = getValidatedInput(
        "Enter green signal duration (1-120 seconds): ",
        1, MAX_SIGNAL_DURATION);

    printf("\nConfiguration saved for this run.\n");
    printf("Traffic density: %d vehicles per direction\n", density);
    printf("Green signal duration: %d seconds per direction\n",
           signalDuration);

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        queue_init(&queues[i]);
    }

    for (i = 0; i < NUM_DIRECTIONS; i++) {
        for (j = 0; j < density; j++) {
            Vehicle arrivingVehicle = { nextVehicleId++, 0 };

            if (!enqueue(&queues[i], arrivingVehicle)) {
                fprintf(stderr,
                        "Error: Queue full for direction %s.\n",
                        directionNames[i]);
                return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}
