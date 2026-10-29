#ifndef SIMULATION_H
#define SIMULATION_H

#include <stddef.h>

#include "queue.h"
#include "stats.h"

#define SIM_MIN_DIRECTIONS 2
#define SIM_MAX_DIRECTIONS 4
#define SIM_MAX_ARRIVAL_INTERVAL 1000
#define SIM_MIN_GREEN_TICKS 1
#define SIM_MAX_GREEN_TICKS 120
#define SIM_MIN_YELLOW_TICKS 1
#define SIM_MAX_YELLOW_TICKS 30
#define SIM_MIN_DISCHARGE 1
#define SIM_MAX_DISCHARGE 10
#define SIM_MIN_TICKS 1
#define SIM_MAX_TICKS 10000

typedef enum {
    SIM_SIGNAL_RED = 0,
    SIM_SIGNAL_GREEN,
    SIM_SIGNAL_YELLOW
} SimSignal;

/*
 * Time is measured in logical ticks (1 tick = 1 simulated second); ticks are
 * numbered from 0. Each direction has its own bounded queue. Directions take
 * turns: one is GREEN for green_ticks, then YELLOW for yellow_ticks, while all
 * others are RED.
 */
typedef struct {
    int num_directions;      /* SIM_MIN_DIRECTIONS .. SIM_MAX_DIRECTIONS */
    int initial_vehicles;    /* waiting in every direction at tick 0 (0 .. QUEUE_CAPACITY) */
    int arrival_interval;    /* afterwards each direction gets one vehicle every N ticks; 0 = none */
    int green_ticks;
    int yellow_ticks;
    int discharge_per_tick;  /* vehicles that can leave per green tick */
    int total_ticks;
} SimConfig;

/* Snapshot passed to the observer after every tick. */
typedef struct {
    unsigned int tick;
    int num_directions;
    SimSignal signal[SIM_MAX_DIRECTIONS];
    int queue_length[SIM_MAX_DIRECTIONS];  /* at the end of the tick */
    int departed[SIM_MAX_DIRECTIONS];      /* vehicles that passed this tick */
    int dropped[SIM_MAX_DIRECTIONS];       /* arrivals rejected this tick (queue full) */
} SimTickInfo;

typedef void (*SimTickCallback)(const SimTickInfo *info, void *user);

void sim_config_default(SimConfig *config);

/*
 * Returns 1 if every field is in range. Otherwise returns 0 and, when error is
 * not NULL, writes a short message naming the offending field.
 */
int sim_config_validate(const SimConfig *config, char *error, size_t error_size);

/* Signal shown to one direction at a tick (RED for any invalid argument). */
SimSignal sim_signal_at(const SimConfig *config, int direction, unsigned int tick);
const char *sim_signal_name(SimSignal signal);

/*
 * Runs one simulation: arrivals, then departures, in every tick. Fills stats.
 * callback may be NULL. Returns 1 on success, 0 on invalid arguments or config.
 */
int sim_run(const SimConfig *config, SimStats *stats,
            SimTickCallback callback, void *user);

#endif
