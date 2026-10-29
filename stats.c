#include "stats.h"

#include <string.h>

void stats_init(SimStats *stats)
{
    if (stats != NULL) {
        memset(stats, 0, sizeof *stats);
    }
}

void stats_record_generated(SimStats *stats)
{
    if (stats != NULL) {
        stats->generated++;
    }
}

void stats_record_departure(SimStats *stats, unsigned int wait_ticks)
{
    if (stats != NULL) {
        stats->passed++;
        stats->total_wait += wait_ticks;
    }
}

void stats_record_queue_length(SimStats *stats, int length)
{
    if (stats != NULL && length > stats->max_queue_length) {
        stats->max_queue_length = length;
    }
}

double stats_average_wait(const SimStats *stats)
{
    if (stats == NULL || stats->passed == 0) {
        return 0.0;
    }

    return (double)stats->total_wait / (double)stats->passed;
}

int stats_is_conserved(const SimStats *stats)
{
    return stats != NULL &&
           stats->generated ==
               stats->passed + stats->dropped + stats->remaining;
}

int report_format(char *buf, size_t size, const SimStats *stats)
{
    if (stats == NULL || (buf == NULL && size != 0)) {
        return -1;
    }

    return snprintf(buf, size,
        "========== SIMULATION SUMMARY ==========\n"
        "Ticks simulated       : %u\n"
        "Vehicles generated    : %lu\n"
        "Vehicles processed    : %lu\n"
        "Vehicles dropped      : %lu (queue full)\n"
        "Vehicles still queued : %lu\n"
        "Average wait (ticks)  : %.2f\n"
        "Maximum queue length  : %d\n"
        "Integrity check       : %s\n"
        "========================================\n",
        stats->ticks,
        stats->generated,
        stats->passed,
        stats->dropped,
        stats->remaining,
        stats_average_wait(stats),
        stats->max_queue_length,
        stats_is_conserved(stats) ? "OK" : "FAILED");
}

void report_print(FILE *out, const SimStats *stats)
{
    char text[512];

    if (out == NULL) {
        return;
    }

    if (report_format(text, sizeof text, stats) >= 0) {
        fputs(text, out);
    }
}
