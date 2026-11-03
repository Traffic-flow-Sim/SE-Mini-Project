#ifndef SIMULATION_H
#define SIMULATION_H

#include <stddef.h>

#include "queue.h"
#include "stats.h"
#include "vehicle_generator.h"

#define SIM_MIN_DIRECTIONS 2
#define SIM_MAX_DIRECTIONS VG_MAX_DIRECTIONS
#define SIM_MAX_ARRIVAL_RATE VG_MAX_RATE
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
 * numbered from 0. Directions are numbered 0..3 (North, South, East, West) and
 * each has its own bounded queue. Directions take turns: one is GREEN for
 * green_ticks, then YELLOW for yellow_ticks, while all others are RED.
 *
 * Every tick, vehicles arrive first and then the green direction releases
 * vehicles. Arrivals come from the VehicleGenerator: at tick 0 the initial
 * vehicles are placed in every queue, and from then on each direction receives
 * vehicles at its own traffic density (arrival_rate).
 */
typedef struct {
    int num_directions;      /* SIM_MIN_DIRECTIONS .. SIM_MAX_DIRECTIONS */
    int initial_vehicles;    /* waiting in every direction at tick 0 (0 .. QUEUE_CAPACITY) */
    int arrival_rate[SIM_MAX_DIRECTIONS]; /* traffic density: new vehicles per 100 ticks, per
                                             direction, 0 .. SIM_MAX_ARRIVAL_RATE.
                                             Only the first num_directions entries are used. */
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
    int remaining_duration[SIM_MAX_DIRECTIONS]; /* remaining ticks in the current signal state */
    int generated[SIM_MAX_DIRECTIONS];     /* vehicles created for this direction this tick
                                              (including any dropped because the queue was full) */
    int queue_length[SIM_MAX_DIRECTIONS];  /* at the end of the tick */
    int departed[SIM_MAX_DIRECTIONS];      /* vehicles that passed this tick */
    unsigned int departed_id[SIM_MAX_DIRECTIONS][SIM_MAX_DISCHARGE]; /* their IDs, oldest first;
                                              only the first departed[d] entries are meaningful */
    int dropped[SIM_MAX_DIRECTIONS];       /* arrivals rejected this tick (queue full) */
} SimTickInfo;

typedef void (*SimTickCallback)(const SimTickInfo *info, void *user);

void sim_config_default(SimConfig *config);

/* Sets the same traffic density (vehicles per 100 ticks) for every direction. */
void sim_config_set_traffic_density(SimConfig *config, int vehicles_per_100_ticks);

/*
 * Returns 1 if every field is in range. Otherwise returns 0 and, when error is
 * not NULL, writes a short message naming the offending field.
 */
int sim_config_validate(const SimConfig *config, char *error, size_t error_size);

/* Signal shown to one direction at a tick (RED for any invalid argument). */
SimSignal sim_signal_at(const SimConfig *config, int direction, unsigned int tick);

/* Remaining ticks in that direction's current signal state at the given tick. */
int sim_signal_remaining_at(const SimConfig *config, int direction, unsigned int tick);

const char *sim_signal_name(SimSignal signal);

/*
 * Runs one simulation: arrivals, then departures, in every tick. Fills stats.
 * callback may be NULL. Returns 1 on success, 0 on invalid arguments or config.
 */
int sim_run(const SimConfig *config, SimStats *stats,
            SimTickCallback callback, void *user);

#endif
