#include "simulation.h"

#include <stdio.h>
#include <string.h>

#include "vehicle_generator.h"

void sim_config_default(SimConfig *config)
{
    if (config == NULL) {
        return;
    }

    config->num_directions = SIM_MAX_DIRECTIONS;
    config->initial_vehicles = 10;
    config->green_ticks = 5;
    config->yellow_ticks = 1;
    config->discharge_per_tick = 1;
    config->total_ticks = 120;
    sim_config_set_traffic_density(config, 20);   /* one vehicle every 5 ticks per direction */
}

void sim_config_set_traffic_density(SimConfig *config, int vehicles_per_100_ticks)
{
    int d;

    if (config == NULL) {
        return;
    }

    for (d = 0; d < SIM_MAX_DIRECTIONS; d++) {
        config->arrival_rate[d] = vehicles_per_100_ticks;
    }
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

static int densities_in_range(const SimConfig *config, char *error, size_t error_size)
{
    int d;

    for (d = 0; d < config->num_directions; d++) {
        if (!in_range("traffic density", config->arrival_rate[d],
                      0, SIM_MAX_ARRIVAL_RATE, error, error_size)) {
            return 0;
        }
    }

    return 1;
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

    /* num_directions is checked first: the density check below relies on it. */
    return in_range("number of directions", config->num_directions,
                    SIM_MIN_DIRECTIONS, SIM_MAX_DIRECTIONS, error, error_size)
        && in_range("initial vehicles", config->initial_vehicles,
                    0, QUEUE_CAPACITY, error, error_size)
        && densities_in_range(config, error, error_size)
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
        config->num_directions < SIM_MIN_DIRECTIONS ||
        config->num_directions > SIM_MAX_DIRECTIONS ||
        config->green_ticks < SIM_MIN_GREEN_TICKS ||
        config->green_ticks > SIM_MAX_GREEN_TICKS ||
        config->yellow_ticks < SIM_MIN_YELLOW_TICKS ||
        config->yellow_ticks > SIM_MAX_YELLOW_TICKS) {
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

/* Returns the ticks left in this signal state, or until this direction is green. */
int sim_signal_remaining_at(const SimConfig *config, int direction, unsigned int tick)
{
    int phase_length;
    int cycle_length;
    int position;
    int active_direction;
    int within_phase;
    int direction_start;

    if (config == NULL || direction < 0 || direction >= config->num_directions ||
        config->num_directions < SIM_MIN_DIRECTIONS ||
        config->num_directions > SIM_MAX_DIRECTIONS ||
        config->green_ticks < SIM_MIN_GREEN_TICKS ||
        config->green_ticks > SIM_MAX_GREEN_TICKS ||
        config->yellow_ticks < SIM_MIN_YELLOW_TICKS ||
        config->yellow_ticks > SIM_MAX_YELLOW_TICKS) {
        return 0;
    }

    phase_length = config->green_ticks + config->yellow_ticks;
    cycle_length = phase_length * config->num_directions;
    position = (int)(tick % (unsigned int)cycle_length);
    active_direction = position / phase_length;
    within_phase = position % phase_length;

    if (direction == active_direction) {
        return within_phase < config->green_ticks
            ? config->green_ticks - within_phase
            : config->yellow_ticks - (within_phase - config->green_ticks);
    }

    direction_start = direction * phase_length;
    return (direction_start - position + cycle_length) % cycle_length;
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

/*
 * Puts a freshly generated vehicle into the queue of its own direction and
 * records it. The queue is chosen by the caller from the direction the vehicle
 * was generated for, so vehicles can never end up in another direction's queue.
 */
static void admit_vehicle(VehicleQueue *queue, Vehicle vehicle, int direction,
                          SimStats *stats, SimTickInfo *info)
{
    stats_record_generated(stats);
    info->generated[direction]++;

    if (!enqueue(queue, vehicle)) {
        info->dropped[direction]++;
    }
}

static void session_finish(SimSession *session, SimSessionState state)
{
    int d;

    if (session->state == SIM_SESSION_STOPPED ||
        session->state == SIM_SESSION_COMPLETED) {
        return;
    }
    for (d = 0; d < session->config.num_directions; d++) {
        session->stats->dropped += session->queues[d].dropped;
        session->stats->remaining += (unsigned long)session->queues[d].size;
    }
    session->stats->ticks = session->next_tick;
    session->state = state;
}

int sim_session_start(SimSession *session, const SimConfig *config,
                      SimStats *stats, SimTickCallback callback, void *user)
{
    int d;

    if (session == NULL || config == NULL || stats == NULL ||
        (session->state != SIM_SESSION_IDLE &&
         session->state != SIM_SESSION_STOPPED &&
         session->state != SIM_SESSION_COMPLETED) ||
        !sim_config_validate(config, NULL, 0) ||
        !vg_init(&session->generator, config->num_directions, config->arrival_rate)) {
        return 0;
    }
    session->config = *config;
    session->stats = stats;
    session->callback = callback;
    session->user = user;
    session->next_tick = 0;
    stats_init(stats);
    for (d = 0; d < config->num_directions; d++) {
        queue_init(&session->queues[d]);
    }
    session->state = SIM_SESSION_RUNNING;
    return 1;
}

int sim_session_pause(SimSession *session)
{
    if (session == NULL || session->state != SIM_SESSION_RUNNING) return 0;
    session->state = SIM_SESSION_PAUSED;
    return 1;
}

int sim_session_resume(SimSession *session)
{
    if (session == NULL || session->state != SIM_SESSION_PAUSED) return 0;
    session->state = SIM_SESSION_RUNNING;
    return 1;
}

int sim_session_stop(SimSession *session)
{
    if (session == NULL || (session->state != SIM_SESSION_RUNNING &&
        session->state != SIM_SESSION_PAUSED)) return 0;
    session_finish(session, SIM_SESSION_STOPPED);
    return 1;
}

int sim_session_step(SimSession *session)
{
    SimTickInfo info;
    Vehicle vehicle;
    unsigned int tick;
    int d;
    int k;

    if (session == NULL || session->state != SIM_SESSION_RUNNING) {
        return 0;
    }

    if (session->next_tick >= (unsigned int)session->config.total_ticks) {
        session_finish(session, SIM_SESSION_COMPLETED);
        return 0;
    }
    tick = session->next_tick;
    memset(&info, 0, sizeof info);
    info.tick = tick;
    info.num_directions = session->config.num_directions;

    /* 1. Arrivals. The initial vehicles are placed at tick 0, then every
     *    direction receives vehicles at its own configured density. */
    if (tick == 0) {
        for (d = 0; d < session->config.num_directions; d++) {
            for (k = 0; k < session->config.initial_vehicles; k++) {
                admit_vehicle(&session->queues[d],
                              vg_new_vehicle(&session->generator, tick),
                              d, session->stats, &info);
            }
        }
    }
    for (d = 0; d < session->config.num_directions; d++) {
        if (vg_arrival(&session->generator, d, tick, &vehicle)) {
            admit_vehicle(&session->queues[d], vehicle, d, session->stats, &info);
        }
    }

    /* 2. The queue peak is sampled after arrivals, before anyone leaves. */
    for (d = 0; d < session->config.num_directions; d++) {
        stats_record_queue_length(session->stats, session->queues[d].size);
    }

    /* 3. Departures: only the green direction releases vehicles, oldest first. */
    for (d = 0; d < session->config.num_directions; d++) {
        info.signal[d] = sim_signal_at(&session->config, d, tick);
        info.remaining_duration[d] =
            sim_signal_remaining_at(&session->config, d, tick);

        if (info.signal[d] == SIM_SIGNAL_GREEN) {
            for (k = 0; k < session->config.discharge_per_tick; k++) {
                if (!dequeue(&session->queues[d], &vehicle)) {
                    break;
                }
                stats_record_departure(session->stats, tick - vehicle.arrival_tick);
                info.departed_id[d][info.departed[d]] = vehicle.id;
                info.departed[d]++;
            }
        }
        info.queue_length[d] = session->queues[d].size;
    }

    if (session->callback != NULL) {
        session->callback(&info, session->user);
    }
    session->next_tick++;
    if (session->next_tick == (unsigned int)session->config.total_ticks) {
        session_finish(session, SIM_SESSION_COMPLETED);
    }
    return 1;
}

int sim_run(const SimConfig *config, SimStats *stats,
            SimTickCallback callback, void *user)
{
    SimSession session = {0};

    if (!sim_session_start(&session, config, stats, callback, user)) {
        return 0;
    }
    while (session.state == SIM_SESSION_RUNNING) {
        (void)sim_session_step(&session);
    }
    return 1;
}
