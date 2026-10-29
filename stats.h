#ifndef STATS_H
#define STATS_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    unsigned long generated;   /* vehicles created (every one is queued or dropped) */
    unsigned long passed;      /* vehicles that left on green */
    unsigned long dropped;     /* vehicles rejected because their queue was full */
    unsigned long remaining;   /* vehicles still queued when the run ended */
    unsigned long total_wait;  /* sum of (departure tick - arrival tick) over passed vehicles */
    int max_queue_length;      /* longest single queue observed during the run */
    unsigned int ticks;        /* ticks simulated */
} SimStats;

void stats_init(SimStats *stats);
void stats_record_generated(SimStats *stats);
void stats_record_departure(SimStats *stats, unsigned int wait_ticks);
void stats_record_queue_length(SimStats *stats, int length);

/* Average wait in ticks; 0.0 when no vehicle has passed. */
double stats_average_wait(const SimStats *stats);

/* Integrity check: generated == passed + dropped + remaining. */
int stats_is_conserved(const SimStats *stats);

/*
 * Formats the end-of-run summary into buf (always NUL-terminated when size > 0).
 * Returns the length the full text needs, like snprintf, or -1 on bad arguments.
 */
int report_format(char *buf, size_t size, const SimStats *stats);
void report_print(FILE *out, const SimStats *stats);

#endif
