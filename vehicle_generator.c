#include "vehicle_generator.h"

#include <stddef.h>

int vg_init(VehicleGenerator *vg, int num_directions, const int *rates)
{
    int d;

    if (vg == NULL || rates == NULL ||
        num_directions < 1 || num_directions > VG_MAX_DIRECTIONS) {
        return 0;
    }

    for (d = 0; d < num_directions; d++) {
        if (rates[d] < 0 || rates[d] > VG_MAX_RATE) {
            return 0;
        }
    }

    vg->next_id = 1;
    vg->num_directions = num_directions;
    for (d = 0; d < VG_MAX_DIRECTIONS; d++) {
        vg->rate[d] = d < num_directions ? rates[d] : 0;
        vg->credit[d] = 0;
    }

    return 1;
}

Vehicle vg_new_vehicle(VehicleGenerator *vg, unsigned int tick)
{
    Vehicle vehicle;

    vehicle.id = 0;
    vehicle.arrival_tick = tick;

    if (vg != NULL) {
        vehicle.id = vg->next_id++;
    }

    return vehicle;
}

int vg_arrival(VehicleGenerator *vg, int direction, unsigned int tick, Vehicle *out)
{
    if (vg == NULL || out == NULL ||
        direction < 0 || direction >= vg->num_directions) {
        return 0;
    }

    /* Each tick adds the rate to this direction's credit; every full
     * VG_RATE_SCALE of credit buys one vehicle. */
    vg->credit[direction] += vg->rate[direction];
    if (vg->credit[direction] < VG_RATE_SCALE) {
        return 0;
    }

    vg->credit[direction] -= VG_RATE_SCALE;
    *out = vg_new_vehicle(vg, tick);

    return 1;
}
