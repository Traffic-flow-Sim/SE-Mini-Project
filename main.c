
#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "queue.h"
#include "simulation.h"
#include "stats.h"

#define NUM_DIRECTIONS SIM_MAX_DIRECTIONS
#define MAX_SIGNAL_DURATION SIM_MAX_GREEN_TICKS
#define MIN_RUN_TICKS 60   /* one tick is one simulated second */

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

/* Called by the simulation after every tick: signal and queue length per direction. */
static void displayTick(const SimTickInfo *info, void *user)
{
    int i;
    int arrived = 0;
    int passed = 0;
    int dropped = 0;

    (void)user;

    printf("Tick %03u", info->tick);

    for (i = 0; i < info->num_directions; i++) {
        printf(" | %-5s %-6s q=%-3d",
               directionNames[i],
               sim_signal_name(info->signal[i]),
               info->queue_length[i]);
        arrived += info->generated[i];
        passed += info->departed[i];
        dropped += info->dropped[i];
    }

    printf(" || arrived %d, passed %d, dropped %d\n", arrived, passed, dropped);
}

/* Long enough for every direction to get at least two green phases. */
static int chooseRunLength(int greenTicks, int yellowTicks)
{
    int twoCycles = 2 * NUM_DIRECTIONS * (greenTicks + yellowTicks);

    return twoCycles > MIN_RUN_TICKS ? twoCycles : MIN_RUN_TICKS;
}

int main(void)
{
    SimConfig config;
    SimStats stats;
    char error[96];
    int initialVehicles;
    int signalDuration;
    int trafficDensity;

    printf("=== TRAFFIC MANAGEMENT CONFIGURATION ===\n");

    initialVehicles = getValidatedInput(
        "Enter initial vehicles per direction (1-100): ",
        1, QUEUE_CAPACITY);

    signalDuration = getValidatedInput(
        "Enter green signal duration (1-120 seconds): ",
        1, MAX_SIGNAL_DURATION);

    trafficDensity = getValidatedInput(
        "Enter traffic density (0-100 vehicles per direction per 100 seconds): ",
        0, SIM_MAX_ARRIVAL_RATE);

    sim_config_default(&config);
    config.num_directions = NUM_DIRECTIONS;
    config.initial_vehicles = initialVehicles;
    config.green_ticks = signalDuration;
    config.total_ticks = chooseRunLength(signalDuration, config.yellow_ticks);
    sim_config_set_traffic_density(&config, trafficDensity);

    if (!sim_config_validate(&config, error, sizeof error)) {
        fprintf(stderr, "Error: %s.\n", error);
        return EXIT_FAILURE;
    }

    printf("\nConfiguration saved for this run.\n");
    printf("Initial vehicles: %d per direction\n", initialVehicles);
    printf("Green signal duration: %d seconds per direction\n",
           signalDuration);
    printf("Traffic density: %d vehicles per direction per 100 seconds\n",
           trafficDensity);
    printf("Simulated time: %d seconds\n", config.total_ticks);

    printf("\nTRAFFIC MANAGEMENT SIMULATION\n\n");

    if (!sim_run(&config, &stats, displayTick, NULL)) {
        fprintf(stderr, "Error: Simulation could not run.\n");
        return EXIT_FAILURE;
    }

    printf("\n");
    report_print(stdout, &stats);

    return stats_is_conserved(&stats) ? EXIT_SUCCESS : EXIT_FAILURE;
}
