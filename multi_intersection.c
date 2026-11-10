#include "multi_intersection.h"

#include <string.h>

/*
 * Build an up-to-date statistics snapshot.
 * Active sessions have not yet finalized their dropped and remaining counts,
 * so calculate those values directly from their queues.
 */
static SimStats intersection_stats_snapshot(const SimIntersection *intersection)
{
    SimStats snapshot;
    int d;

    snapshot = intersection->stats;

    if (intersection->session.state == SIM_SESSION_RUNNING ||
        intersection->session.state == SIM_SESSION_PAUSED) {
        snapshot.dropped = 0;
        snapshot.remaining = 0;
        snapshot.ticks = intersection->session.next_tick;

        for (d = 0; d < intersection->session.config.num_directions; d++) {
            snapshot.dropped +=
                (unsigned long)intersection->session.queues[d].dropped;
            snapshot.remaining +=
                (unsigned long)intersection->session.queues[d].size;
        }
    }

    return snapshot;
}

/* Recalculate the combined statistics from all intersections. */
static void update_combined_stats(SimMultiIntersection *manager)
{
    int i;

    stats_init(&manager->combined_stats);

    for (i = 0; i < manager->count; i++) {
        SimStats current =
            intersection_stats_snapshot(&manager->intersections[i]);

        manager->combined_stats.generated += current.generated;
        manager->combined_stats.passed += current.passed;
        manager->combined_stats.dropped += current.dropped;
        manager->combined_stats.remaining += current.remaining;
        manager->combined_stats.total_wait += current.total_wait;

        if (current.max_queue_length >
            manager->combined_stats.max_queue_length) {
            manager->combined_stats.max_queue_length =
                current.max_queue_length;
        }

        if (current.ticks > manager->combined_stats.ticks) {
            manager->combined_stats.ticks = current.ticks;
        }
    }
}

int sim_multi_init(SimMultiIntersection *manager, int count,
                   const SimConfig configs[])
{
    int i;

    if (manager == NULL || configs == NULL ||
        count < SIM_MIN_INTERSECTIONS ||
        count > SIM_MAX_INTERSECTIONS) {
        return 0;
    }

    /* Validate every configuration before changing the manager. */
    for (i = 0; i < count; i++) {
        if (!sim_config_validate(&configs[i], NULL, 0)) {
            return 0;
        }
    }

    memset(manager, 0, sizeof *manager);
    manager->count = count;

    for (i = 0; i < count; i++) {
        manager->intersections[i].id = i + 1;
        manager->intersections[i].config = configs[i];
        stats_init(&manager->intersections[i].stats);
    }

    stats_init(&manager->combined_stats);
    return 1;
}

int sim_multi_start(SimMultiIntersection *manager)
{
    int i;

    if (manager == NULL || manager->count < SIM_MIN_INTERSECTIONS ||
        manager->count > SIM_MAX_INTERSECTIONS) {
        return 0;
    }

    for (i = 0; i < manager->count; i++) {
        SimIntersection *intersection = &manager->intersections[i];

        if (!sim_session_start(&intersection->session,
                               &intersection->config,
                               &intersection->stats,
                               NULL, NULL)) {
            /* Roll back sessions already started in this operation. */
            while (i > 0) {
                i--;
                (void)sim_session_stop(
                    &manager->intersections[i].session);
            }
            update_combined_stats(manager);
            return 0;
        }
    }

    update_combined_stats(manager);
    return 1;
}

int sim_multi_step(SimMultiIntersection *manager)
{
    int i;
    int advanced = 0;

    if (manager == NULL || manager->count < SIM_MIN_INTERSECTIONS ||
        manager->count > SIM_MAX_INTERSECTIONS) {
        return 0;
    }

    for (i = 0; i < manager->count; i++) {
        if (sim_session_step(&manager->intersections[i].session)) {
            advanced = 1;
        }
    }

    update_combined_stats(manager);
    return advanced;
}

SimIntersection *sim_multi_get(SimMultiIntersection *manager,
                               int intersection_id)
{
    if (manager == NULL || intersection_id < 1 ||
        intersection_id > manager->count ||
        manager->count > SIM_MAX_INTERSECTIONS) {
        return NULL;
    }

    return &manager->intersections[intersection_id - 1];
}

int sim_multi_pause(SimMultiIntersection *manager, int intersection_id)
{
    SimIntersection *intersection =
        sim_multi_get(manager, intersection_id);

    if (intersection == NULL ||
        !sim_session_pause(&intersection->session)) {
        return 0;
    }

    update_combined_stats(manager);
    return 1;
}

int sim_multi_resume(SimMultiIntersection *manager, int intersection_id)
{
    SimIntersection *intersection =
        sim_multi_get(manager, intersection_id);

    if (intersection == NULL ||
        !sim_session_resume(&intersection->session)) {
        return 0;
    }

    update_combined_stats(manager);
    return 1;
}

int sim_multi_stop(SimMultiIntersection *manager, int intersection_id)
{
    SimIntersection *intersection =
        sim_multi_get(manager, intersection_id);

    if (intersection == NULL ||
        !sim_session_stop(&intersection->session)) {
        return 0;
    }

    update_combined_stats(manager);
    return 1;
}

const SimStats *sim_multi_get_combined_stats(SimMultiIntersection *manager)
{
    if (manager == NULL || manager->count < SIM_MIN_INTERSECTIONS ||
        manager->count > SIM_MAX_INTERSECTIONS) {
        return NULL;
    }

    update_combined_stats(manager);
    return &manager->combined_stats;
}

int sim_multi_is_conserved(SimMultiIntersection *manager)
{
    int i;
    const SimStats *combined;

    if (manager == NULL || manager->count < SIM_MIN_INTERSECTIONS ||
        manager->count > SIM_MAX_INTERSECTIONS) {
        return 0;
    }

    for (i = 0; i < manager->count; i++) {
        SimStats current =
            intersection_stats_snapshot(&manager->intersections[i]);

        if (!stats_is_conserved(&current)) {
            return 0;
        }
    }

    combined = sim_multi_get_combined_stats(manager);
    return stats_is_conserved(combined);
}
