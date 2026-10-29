#include "simulation.h"

#include <stdio.h>
#include <string.h>

void sim_config_default(SimConfig *config)
{
    if (config == NULL) {
        return;
    }

    config->num_directions = SIM_MAX_DIRECTIONS;
    config->initial_vehicles = 10;
    config->arrival_interval = 5;
    config->green_ticks = 5;
    config->yellow_ticks = 1;
    config->discharge_per_tick = 1;
    config->total_ticks = 120;
}

static int in_range(const char *name, int value, int min, int max,
                    char *error, size_t error_size)
{
    if (value >= min && value <= max) {
        return 1;
    }

    if (error != NULL && error_size > 0) {
        snprintf(error, error_size, "%s must be between %d and %d",
                 name, min, max);
    }

    return 0;
}

int sim_config_validate(const SimConfig *config, char *error, size_t error_size)
{
    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }

    if (config == NULL) {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "configuration is missing");
        }
        return 0;
    }

    return in_range("number of directions", config->num_directions,
                    SIM_MIN_DIRECTIONS, SIM_MAX_DIRECTIONS, error, error_size)
        && in_range("initial vehicles", config->initial_vehicles,
                    0, QUEUE_CAPACITY, error, error_size)
        && in_range("arrival interval", config->arrival_interval,
                    0, SIM_MAX_ARRIVAL_INTERVAL, error, error_size)
        && in_range("green duration", config->green_ticks,
                    SIM_MIN_GREEN_TICKS, SIM_MAX_GREEN_TICKS, error, error_size)
        && in_range("yellow duration", config->yellow_ticks,
                    SIM_MIN_YELLOW_TICKS, SIM_MAX_YELLOW_TICKS, error, error_size)
        && in_range("vehicles released per green tick", config->discharge_per_tick,
                    SIM_MIN_DISCHARGE, SIM_MAX_DISCHARGE, error, error_size)
        && in_range("simulated ticks", config->total_ticks,
                    SIM_MIN_TICKS, SIM_MAX_TICKS, error, error_size);
}

SimSignal sim_signal_at(const SimConfig *config, int direction, unsigned int tick)
{
    unsigned int phase_length;
    unsigned int position;
    int active;
    int within;

    if (config == NULL || direction < 0 || direction >= config->num_directions ||
        config->green_ticks < 1 || config->yellow_ticks < 1) {
        return SIM_SIGNAL_RED;
    }

    phase_length = (unsigned int)(config->green_ticks + config->yellow_ticks);
    position = tick % (phase_length * (unsigned int)config->num_directions);
    active = (int)(position / phase_length);
    within = (int)(position % phase_length);

    if (direction != active) {
        return SIM_SIGNAL_RED;
    }

    return within < config->green_ticks ? SIM_SIGNAL_GREEN : SIM_SIGNAL_YELLOW;
}

const char *sim_signal_name(SimSignal signal)
{
    switch (signal) {
        case SIM_SIGNAL_GREEN:  return "GREEN";
        case SIM_SIGNAL_YELLOW: return "YELLOW";
        case SIM_SIGNAL_RED:    return "RED";
        default:                return "UNKNOWN";
    }
}

/* Creates the next vehicle and queues it; returns 0 if the queue dropped it. */
static int vehicle_arrives(VehicleQueue *queue, unsigned int *next_id,
                           unsigned int tick, SimStats *stats)
{
    Vehicle vehicle;

    vehicle.id = (*next_id)++;
    vehicle.arrival_tick = tick;
    stats_record_generated(stats);

    return enqueue(queue, vehicle);
}

int sim_run(const SimConfig *config, SimStats *stats,
            SimTickCallback callback, void *user)
{
    VehicleQueue queues[SIM_MAX_DIRECTIONS];
    SimTickInfo info;
    unsigned int next_id = 1;
    unsigned int tick;
    int d;
    int k;

    if (config == NULL || stats == NULL ||
        !sim_config_validate(config, NULL, 0)) {
        return 0;
    }

    stats_init(stats);
    for (d = 0; d < config->num_directions; d++) {
        queue_init(&queues[d]);
    }

    for (tick = 0; tick < (unsigned int)config->total_ticks; tick++) {
        memset(&info, 0, sizeof info);
        info.tick = tick;
        info.num_directions = config->num_directions;

        /* 1. Arrivals: the initial vehicles at tick 0, then one per direction per interval. */
        if (tick == 0) {
            for (d = 0; d < config->num_directions; d++) {
                for (k = 0; k < config->initial_vehicles; k++) {
                    if (!vehicle_arrives(&queues[d], &next_id, tick, stats)) {
                        info.dropped[d]++;
                    }
                }
            }
        } else if (config->arrival_interval > 0 &&
                   tick % (unsigned int)config->arrival_interval == 0) {
            for (d = 0; d < config->num_directions; d++) {
                if (!vehicle_arrives(&queues[d], &next_id, tick, stats)) {
                    info.dropped[d]++;
                }
            }
        }

        /* 2. The queue peak is sampled after arrivals, before anyone leaves. */
        for (d = 0; d < config->num_directions; d++) {
            stats_record_queue_length(stats, queues[d].size);
        }

        /* 3. Departures: only the green direction releases vehicles, oldest first. */
        for (d = 0; d < config->num_directions; d++) {
            info.signal[d] = sim_signal_at(config, d, tick);

            if (info.signal[d] == SIM_SIGNAL_GREEN) {
                for (k = 0; k < config->discharge_per_tick; k++) {
                    Vehicle vehicle;

                    if (!dequeue(&queues[d], &vehicle)) {
                        break;
                    }
                    stats_record_departure(stats, tick - vehicle.arrival_tick);
                    info.departed[d]++;
                }
            }

            info.queue_length[d] = queues[d].size;
        }

        if (callback != NULL) {
            callback(&info, user);
        }
    }

    /* Dropped and remaining come from the queues themselves, so the integrity
     * check in stats_is_conserved compares independent counters. */
    stats->ticks = (unsigned int)config->total_ticks;
    for (d = 0; d < config->num_directions; d++) {
        stats->dropped += queues[d].dropped;
        stats->remaining += (unsigned long)queues[d].size;
    }

    return 1;
}
