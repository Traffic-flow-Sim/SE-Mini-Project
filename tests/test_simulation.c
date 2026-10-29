/*
 * Integration tests: arrivals -> per-direction queues <-> signals -> stats -> report,
 * driven by sim_run (S1TMS-6 per-direction queues and signals, S1TMS-7 integrity,
 * S1TMS-9 report).
 *
 * The expected numbers in the "hand computed" tests are derived on paper from the
 * rules in simulation.h, not copied from program output.
 */
#include <stdio.h>
#include <string.h>

#include "simulation.h"
#include "test_util.h"

static SimConfig make_config(int dirs, int initial, int interval,
                             int green, int yellow, int discharge, int ticks)
{
    SimConfig c;

    c.num_directions = dirs;
    c.initial_vehicles = initial;
    c.arrival_interval = interval;
    c.green_ticks = green;
    c.yellow_ticks = yellow;
    c.discharge_per_tick = discharge;
    c.total_ticks = ticks;
    return c;
}

/* ---- configuration (boundary values: min-1, min, max, max+1) ---- */

static void test_default_config_is_valid(void)
{
    SimConfig c;
    char err[96];

    sim_config_default(&c);
    CHECK(sim_config_validate(&c, err, sizeof err) == 1);
    CHECK(err[0] == '\0');
    sim_config_default(NULL);                       /* must not crash */
}

#define BOUNDARY(field, lo, hi, name)                                         \
    do {                                                                      \
        SimConfig c;                                                          \
        char err[96];                                                         \
        sim_config_default(&c);                                               \
        c.field = (lo);                                                       \
        CHECK(sim_config_validate(&c, err, sizeof err) == 1);                 \
        c.field = (hi);                                                       \
        CHECK(sim_config_validate(&c, err, sizeof err) == 1);                 \
        c.field = (lo) - 1;                                                   \
        CHECK(sim_config_validate(&c, err, sizeof err) == 0);                 \
        CHECK(strstr(err, name) != NULL);                                     \
        c.field = (hi) + 1;                                                   \
        CHECK(sim_config_validate(&c, err, sizeof err) == 0);                 \
        CHECK(strstr(err, name) != NULL);                                     \
    } while (0)

static void test_config_boundaries(void)
{
    BOUNDARY(num_directions, SIM_MIN_DIRECTIONS, SIM_MAX_DIRECTIONS, "directions");
    BOUNDARY(initial_vehicles, 0, QUEUE_CAPACITY, "initial vehicles");
    BOUNDARY(arrival_interval, 0, SIM_MAX_ARRIVAL_INTERVAL, "arrival interval");
    BOUNDARY(green_ticks, SIM_MIN_GREEN_TICKS, SIM_MAX_GREEN_TICKS, "green");
    BOUNDARY(yellow_ticks, SIM_MIN_YELLOW_TICKS, SIM_MAX_YELLOW_TICKS, "yellow");
    BOUNDARY(discharge_per_tick, SIM_MIN_DISCHARGE, SIM_MAX_DISCHARGE, "per green tick");
    BOUNDARY(total_ticks, SIM_MIN_TICKS, SIM_MAX_TICKS, "simulated ticks");
}

static void test_config_validate_edge_cases(void)
{
    SimConfig c;
    char err[96];
    char tiny[6];

    CHECK(sim_config_validate(NULL, err, sizeof err) == 0);
    CHECK(strlen(err) > 0);
    CHECK(sim_config_validate(NULL, NULL, 0) == 0);

    sim_config_default(&c);
    c.green_ticks = -1;
    CHECK(sim_config_validate(&c, NULL, 0) == 0);   /* error buffer is optional */
    CHECK(sim_config_validate(&c, tiny, sizeof tiny) == 0);
    CHECK(strlen(tiny) < sizeof tiny);              /* truncated, terminated */
}

/* ---- signals ---- */

static void test_signal_sequence_for_two_directions(void)
{
    /* green 2, yellow 1: each phase lasts 3 ticks, the cycle lasts 6. */
    static const SimSignal d0[7] = {
        SIM_SIGNAL_GREEN, SIM_SIGNAL_GREEN, SIM_SIGNAL_YELLOW,
        SIM_SIGNAL_RED, SIM_SIGNAL_RED, SIM_SIGNAL_RED, SIM_SIGNAL_GREEN
    };
    static const SimSignal d1[7] = {
        SIM_SIGNAL_RED, SIM_SIGNAL_RED, SIM_SIGNAL_RED,
        SIM_SIGNAL_GREEN, SIM_SIGNAL_GREEN, SIM_SIGNAL_YELLOW, SIM_SIGNAL_RED
    };
    SimConfig c = make_config(2, 0, 0, 2, 1, 1, 10);
    unsigned int t;

    for (t = 0; t < 7; t++) {
        CHECK(sim_signal_at(&c, 0, t) == d0[t]);
        CHECK(sim_signal_at(&c, 1, t) == d1[t]);
    }
}

static void test_one_direction_active_at_a_time_and_all_served(void)
{
    SimConfig c = make_config(4, 0, 0, 5, 2, 1, 10);
    unsigned int t;
    int d;
    int seen_green[4] = { 0, 0, 0, 0 };

    for (t = 0; t < 200; t++) {
        int not_red = 0;

        for (d = 0; d < 4; d++) {
            SimSignal s = sim_signal_at(&c, d, t);

            if (s != SIM_SIGNAL_RED) {
                not_red++;
            }
            if (s == SIM_SIGNAL_GREEN && t < 28) {  /* one full cycle = 4 * 7 */
                seen_green[d] = 1;
            }
        }
        CHECK(not_red == 1);
    }
    for (d = 0; d < 4; d++) {
        CHECK(seen_green[d] == 1);
    }
}

static void test_signal_invalid_arguments_are_red(void)
{
    SimConfig c = make_config(2, 0, 0, 2, 1, 1, 10);

    CHECK(sim_signal_at(&c, -1, 0) == SIM_SIGNAL_RED);
    CHECK(sim_signal_at(&c, 2, 0) == SIM_SIGNAL_RED);
    CHECK(sim_signal_at(NULL, 0, 0) == SIM_SIGNAL_RED);
    c.green_ticks = 0;
    CHECK(sim_signal_at(&c, 0, 0) == SIM_SIGNAL_RED);   /* no division by zero */
    CHECK(strcmp(sim_signal_name(SIM_SIGNAL_GREEN), "GREEN") == 0);
    CHECK(strcmp(sim_signal_name(SIM_SIGNAL_YELLOW), "YELLOW") == 0);
    CHECK(strcmp(sim_signal_name(SIM_SIGNAL_RED), "RED") == 0);
}

/* ---- hand-computed scenarios ---- */

static void test_hand_computed_initial_vehicles_only(void)
{
    /*
     * 2 directions, 2 vehicles each at tick 0 (ids 1,2 in d0; ids 3,4 in d1), no later
     * arrivals, green 1, yellow 1, one vehicle per green tick, 6 ticks.
     * Signals: t0 d0 G | t1 d0 Y | t2 d1 G | t3 d1 Y | t4 d0 G | t5 d0 Y
     *   t0: id1 leaves (wait 0)   t2: id3 leaves (wait 2)   t4: id2 leaves (wait 4)
     * generated 4, passed 3, dropped 0, remaining 1 (id4)
     * total wait 0+2+4 = 6, average 2.0, peak queue 2 (after the tick-0 arrivals).
     */
    SimConfig c = make_config(2, 2, 0, 1, 1, 1, 6);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.generated == 4);
    CHECK(s.passed == 3);
    CHECK(s.dropped == 0);
    CHECK(s.remaining == 1);
    CHECK(s.total_wait == 6);
    CHECK(s.max_queue_length == 2);
    CHECK(s.ticks == 6);
    CHECK(stats_is_conserved(&s));
}

static void test_hand_computed_with_ongoing_arrivals(void)
{
    /*
     * 2 directions, nobody waiting at tick 0, one new vehicle per direction at t = 2,4,6,8,
     * green 2, yellow 1, one per green tick, 9 ticks.
     * Signals: t0-1 d0 G | t2 d0 Y | t3-4 d1 G | t5 d1 Y | t6-7 d0 G | t8 d0 Y
     * Arrivals (d0 then d1): t2 ids 1,2 | t4 ids 3,4 | t6 ids 5,6 | t8 ids 7,8
     *   t3: id2 leaves (arrived 2, wait 1)    t4: id4 leaves (arrived 4, wait 0)
     *   t6: id1 leaves (arrived 2, wait 4)    t7: id3 leaves (arrived 4, wait 3)
     * generated 8, passed 4, dropped 0, remaining 4 (ids 5,6,7,8)
     * total wait 1+0+4+3 = 8, average 2.0
     * peak queue 3: d0 holds ids 1,3,5 after the t6 arrivals, before id1 leaves.
     */
    SimConfig c = make_config(2, 0, 2, 2, 1, 1, 9);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.generated == 8);
    CHECK(s.passed == 4);
    CHECK(s.dropped == 0);
    CHECK(s.remaining == 4);
    CHECK(s.total_wait == 8);
    CHECK(s.max_queue_length == 3);
    CHECK(stats_is_conserved(&s));
}

static void test_hand_computed_multiple_departures_per_green_tick(void)
{
    /*
     * 2 directions, 10 vehicles each at tick 0, no later arrivals, green 2, yellow 1,
     * up to 3 vehicles per green tick, 6 ticks.
     * t0: d0 releases 3 (wait 0)   t1: d0 releases 3 (wait 1)
     * t3: d1 releases 3 (wait 3)   t4: d1 releases 3 (wait 4)
     * passed 12, remaining 8, total wait 3*0 + 3*1 + 3*3 + 3*4 = 24, average 2.0
     */
    SimConfig c = make_config(2, 10, 0, 2, 1, 3, 6);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.generated == 20);
    CHECK(s.passed == 12);
    CHECK(s.remaining == 8);
    CHECK(s.dropped == 0);
    CHECK(s.total_wait == 24);
    CHECK(s.max_queue_length == 10);
    CHECK(stats_is_conserved(&s));
}

typedef struct {
    int dropped_total;
    int dropped_at_tick[3];
} DropProbe;

static void drop_probe(const SimTickInfo *info, void *user)
{
    DropProbe *p = (DropProbe *)user;
    int d;

    for (d = 0; d < info->num_directions; d++) {
        p->dropped_total += info->dropped[d];
        if (info->tick < 3) {
            p->dropped_at_tick[info->tick] += info->dropped[d];
        }
    }
}

static void test_hand_computed_overflow_drops_and_counts(void)
{
    /*
     * 2 directions, queues pre-filled to capacity (100 each, ids 1-100 and 101-200),
     * a new vehicle per direction every tick, green 1, yellow 1, one per green tick, 3 ticks.
     *   t0: d0 green, id1 leaves (wait 0) -> d0 holds 99
     *   t1: id201 joins d0 (back to 100); id202 is dropped, d1 is full. d0 yellow.
     *   t2: id203 dropped (d0 full), id204 dropped (d1 full). d1 green, id101 leaves (wait 2).
     * generated 200 + 2 + 2 = 204, passed 2, dropped 3, remaining 100 + 99 = 199
     * total wait 0 + 2 = 2, peak queue 100.
     */
    SimConfig c = make_config(2, QUEUE_CAPACITY, 1, 1, 1, 1, 3);
    SimStats s;
    DropProbe p;

    memset(&p, 0, sizeof p);
    CHECK(sim_run(&c, &s, drop_probe, &p) == 1);
    CHECK(s.generated == 204);
    CHECK(s.passed == 2);
    CHECK(s.dropped == 3);
    CHECK(s.remaining == 199);
    CHECK(s.total_wait == 2);
    CHECK(s.max_queue_length == QUEUE_CAPACITY);
    CHECK(stats_is_conserved(&s));
    CHECK(p.dropped_total == 3);                    /* the per-tick view agrees */
    CHECK(p.dropped_at_tick[0] == 0);
    CHECK(p.dropped_at_tick[1] == 1);
    CHECK(p.dropped_at_tick[2] == 2);
}

/* ---- behaviour checked through the per-tick callback ---- */

typedef struct {
    int ticks_seen;
    unsigned int next_tick;
    int bad_tick_order;
    int non_green_departures;
    int non_green_queue_changes;
    int green_count_errors;
    int prev_length[SIM_MAX_DIRECTIONS];
    long departed_total;
    long dropped_total;
    int max_end_length;
    int last_length[SIM_MAX_DIRECTIONS];
} Probe;

static void probe_cb(const SimTickInfo *info, void *user)
{
    Probe *p = (Probe *)user;
    int d;
    int greens = 0;

    p->ticks_seen++;
    if (info->tick != p->next_tick) {
        p->bad_tick_order++;
    }
    p->next_tick = info->tick + 1;

    for (d = 0; d < info->num_directions; d++) {
        if (info->signal[d] == SIM_SIGNAL_GREEN) {
            greens++;
            if (info->queue_length[d] != p->prev_length[d] - info->departed[d]) {
                p->green_count_errors++;
            }
        } else {
            if (info->departed[d] != 0) {
                p->non_green_departures++;
            }
            if (info->queue_length[d] != p->prev_length[d]) {
                p->non_green_queue_changes++;       /* only valid with no arrivals */
            }
        }
        p->departed_total += info->departed[d];
        p->dropped_total += info->dropped[d];
        if (info->queue_length[d] > p->max_end_length) {
            p->max_end_length = info->queue_length[d];
        }
        p->prev_length[d] = info->queue_length[d];
        p->last_length[d] = info->queue_length[d];
    }
    if (greens > 1) {
        p->green_count_errors++;
    }
}

static void test_only_green_direction_moves_and_others_are_unchanged(void)
{
    SimConfig c = make_config(4, 20, 0, 3, 1, 1, 64);   /* no arrivals after tick 0 */
    SimStats s;
    Probe p;
    int d;

    memset(&p, 0, sizeof p);
    for (d = 0; d < 4; d++) {
        p.prev_length[d] = 20;
    }
    CHECK(sim_run(&c, &s, probe_cb, &p) == 1);
    CHECK(p.ticks_seen == 64);
    CHECK(p.bad_tick_order == 0);
    CHECK(p.non_green_departures == 0);             /* RED/YELLOW directions never release */
    CHECK(p.non_green_queue_changes == 0);          /* and their queues stay as they were */
    CHECK(p.green_count_errors == 0);               /* one green at a time, counts consistent */
    CHECK(p.departed_total == (long)s.passed);
}

static void test_report_matches_what_was_observed(void)
{
    SimConfig c = make_config(4, 30, 3, 4, 1, 1, 150);
    SimStats s;
    Probe p;
    unsigned long queued = 0;
    int d;

    memset(&p, 0, sizeof p);
    CHECK(sim_run(&c, &s, probe_cb, &p) == 1);
    CHECK(p.departed_total == (long)s.passed);
    CHECK(p.dropped_total == (long)s.dropped);
    for (d = 0; d < 4; d++) {
        queued += (unsigned long)p.last_length[d];
    }
    CHECK(queued == s.remaining);
    CHECK(s.max_queue_length >= p.max_end_length);  /* peak includes pre-departure arrivals */
    CHECK(s.max_queue_length <= QUEUE_CAPACITY);
    CHECK(s.ticks == 150);
    CHECK(stats_is_conserved(&s));
}

/* ---- integrity across many configurations ---- */

static void test_conservation_across_parameter_sweep(void)
{
    static const int dirs[] = { 2, 3, 4 };
    static const int initial[] = { 0, 1, 50, QUEUE_CAPACITY };
    static const int interval[] = { 0, 1, 3, 7 };
    static const int green[] = { 1, 3 };
    static const int discharge[] = { 1, 3 };
    size_t a, b, c, d, e;
    int runs = 0;

    for (a = 0; a < sizeof dirs / sizeof dirs[0]; a++)
    for (b = 0; b < sizeof initial / sizeof initial[0]; b++)
    for (c = 0; c < sizeof interval / sizeof interval[0]; c++)
    for (d = 0; d < sizeof green / sizeof green[0]; d++)
    for (e = 0; e < sizeof discharge / sizeof discharge[0]; e++) {
        SimConfig cfg = make_config(dirs[a], initial[b], interval[c],
                                    green[d], 1, discharge[e], 200);
        SimStats s;

        CHECK(sim_run(&cfg, &s, NULL, NULL) == 1);
        CHECK(stats_is_conserved(&s));              /* nothing lost, nothing duplicated */
        CHECK(s.max_queue_length <= QUEUE_CAPACITY);
        CHECK(s.passed <= s.generated);
        runs++;
    }
    CHECK(runs == 3 * 4 * 4 * 2 * 2);
}

static void test_light_traffic_never_drops(void)
{
    SimConfig c = make_config(4, 5, 10, 6, 1, 2, 300);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.dropped == 0);
    CHECK(s.passed > 0);
    CHECK(stats_is_conserved(&s));
}

static void test_heavy_traffic_drops_but_stays_consistent(void)
{
    SimConfig c = make_config(4, QUEUE_CAPACITY, 1, 1, 1, 1, 400);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.dropped > 0);
    CHECK(s.max_queue_length == QUEUE_CAPACITY);
    CHECK(stats_is_conserved(&s));
}

/* ---- robustness and repeatability ---- */

static void test_invalid_arguments_are_rejected(void)
{
    SimConfig c = make_config(4, 10, 5, 5, 1, 1, 60);
    SimStats s;

    CHECK(sim_run(NULL, &s, NULL, NULL) == 0);
    CHECK(sim_run(&c, NULL, NULL, NULL) == 0);

    c.num_directions = 1;
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, QUEUE_CAPACITY + 1, 5, 5, 1, 1, 60);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, 10, 5, 0, 1, 1, 60);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, 10, 5, 5, 1, 1, 0);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
}

static void test_runs_are_repeatable(void)
{
    SimConfig c;
    SimStats a, b;
    int i;

    sim_config_default(&c);
    CHECK(sim_run(&c, &a, NULL, NULL) == 1);
    for (i = 0; i < 20; i++) {
        CHECK(sim_run(&c, &b, NULL, NULL) == 1);
        CHECK(memcmp(&a, &b, sizeof a) == 0);       /* identical results every time */
    }
}

static void test_default_run_report_is_consistent(void)
{
    SimConfig c;
    SimStats s;
    char text[512];

    sim_config_default(&c);
    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(report_format(text, sizeof text, &s) > 0);
    CHECK(strstr(text, "Integrity check       : OK") != NULL);
    CHECK(strstr(text, "Average wait (ticks)") != NULL);
    CHECK(strstr(text, "Maximum queue length") != NULL);
}

int main(void)
{
    printf("test_simulation\n");
    TEST_RUN(test_default_config_is_valid);
    TEST_RUN(test_config_boundaries);
    TEST_RUN(test_config_validate_edge_cases);
    TEST_RUN(test_signal_sequence_for_two_directions);
    TEST_RUN(test_one_direction_active_at_a_time_and_all_served);
    TEST_RUN(test_signal_invalid_arguments_are_red);
    TEST_RUN(test_hand_computed_initial_vehicles_only);
    TEST_RUN(test_hand_computed_with_ongoing_arrivals);
    TEST_RUN(test_hand_computed_multiple_departures_per_green_tick);
    TEST_RUN(test_hand_computed_overflow_drops_and_counts);
    TEST_RUN(test_only_green_direction_moves_and_others_are_unchanged);
    TEST_RUN(test_report_matches_what_was_observed);
    TEST_RUN(test_conservation_across_parameter_sweep);
    TEST_RUN(test_light_traffic_never_drops);
    TEST_RUN(test_heavy_traffic_drops_but_stays_consistent);
    TEST_RUN(test_invalid_arguments_are_rejected);
    TEST_RUN(test_runs_are_repeatable);
    TEST_RUN(test_default_run_report_is_consistent);
    return TEST_SUMMARY();
}
