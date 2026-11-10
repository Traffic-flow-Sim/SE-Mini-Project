#ifndef MULTI_INTERSECTION_H
#define MULTI_INTERSECTION_H

#include "simulation.h"
#include "stats.h"

#define SIM_MAX_INTERSECTIONS 3
#define SIM_MIN_INTERSECTIONS 1

typedef struct {
    int id;
    SimConfig config;
    SimSession session;
    SimStats stats;
} SimIntersection;

typedef struct {
    int count;
    SimIntersection intersections[SIM_MAX_INTERSECTIONS];
    SimStats combined_stats;
} SimMultiIntersection;

/* Initialize 1 to 3 independent intersections with separate configurations. */
int sim_multi_init(SimMultiIntersection *manager, int count,
                   const SimConfig configs[]);

/* Start all configured intersections. */
int sim_multi_start(SimMultiIntersection *manager);

/* Advance each running intersection by one tick. */
int sim_multi_step(SimMultiIntersection *manager);

/* Control an individual intersection using its ID (1 to 3). */
int sim_multi_pause(SimMultiIntersection *manager, int intersection_id);
int sim_multi_resume(SimMultiIntersection *manager, int intersection_id);
int sim_multi_stop(SimMultiIntersection *manager, int intersection_id);

/* Retrieve an individual intersection. Returns NULL for an invalid ID. */
SimIntersection *sim_multi_get(SimMultiIntersection *manager,
                               int intersection_id);

/* Calculate and return combined statistics. */
const SimStats *sim_multi_get_combined_stats(SimMultiIntersection *manager);

/* Check vehicle conservation for every intersection and combined totals. */
int sim_multi_is_conserved(SimMultiIntersection *manager);

#endif
