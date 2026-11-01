#ifndef VEHICLE_GENERATOR_H
#define VEHICLE_GENERATOR_H

#include "queue.h"

#define VG_MAX_DIRECTIONS 4   /* North, South, East, West */
#define VG_RATE_SCALE 100     /* rates are expressed per 100 ticks */
#define VG_MAX_RATE 100       /* at most one vehicle per direction per tick */

/*
 * Creates vehicles for every direction of the intersection.
 *
 * Traffic density is a rate per direction: vehicles per 100 ticks (1 tick = 1
 * simulated second). The generator is deterministic, so a run can be checked by
 * hand: after N ticks a direction with rate r has produced exactly
 * floor(N * r / 100) vehicles, spread as evenly as possible. For example, rate
 * 20 produces a vehicle at ticks 4, 9, 14, ...
 *
 * All directions share one ID counter, so every vehicle ID is unique across
 * the whole intersection.
 */
typedef struct {
    unsigned int next_id;
    int num_directions;
    int rate[VG_MAX_DIRECTIONS];
    int credit[VG_MAX_DIRECTIONS];   /* fractional progress towards the next vehicle */
} VehicleGenerator;

/*
 * rates points to num_directions values, each 0 .. VG_MAX_RATE.
 * Returns 1 on success, 0 on invalid arguments (the generator is left unchanged).
 */
int vg_init(VehicleGenerator *vg, int num_directions, const int *rates);

/*
 * Creates a vehicle with the next unique ID, outside the rate mechanism (used
 * for vehicles that are already waiting when a run starts). Returns a vehicle
 * with ID 0 if vg is NULL; real vehicles never have ID 0.
 */
Vehicle vg_new_vehicle(VehicleGenerator *vg, unsigned int tick);

/*
 * Call once per direction per tick. Returns 1 and fills *out when a vehicle
 * arrives in that direction at this tick, otherwise 0. Directions are
 * independent: calling one never affects another's timing.
 */
int vg_arrival(VehicleGenerator *vg, int direction, unsigned int tick, Vehicle *out);

#endif
