
#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include "simulation.h"

int audit_log_signal(
    const char *path,
    int intersection_id,
    unsigned int tick,
    int direction,
    SimSignal previous_signal,
    SimSignal new_signal);

int audit_log_config_change(
    const char *path,
    int intersection_id,
    const char *description);

#endif
