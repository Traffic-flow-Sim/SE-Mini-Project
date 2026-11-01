/*
 * Integration tests: generator -> per-direction queues <-> signals -> stats -> report,
 * driven by sim_run (S1TMS-6 per-direction queues and signals, S1TMS-7 integrity,
 * S1TMS-9 report, vehicle generation per direction at the configured density).
 *
 * The expected numbers in the "hand computed" tests are derived on paper from the
 * rules in simulation.h and vehicle_generator.h, not copied from program output.
 */
#include <stdio.h>
#include <string.h>

#include "simulation.h"
#include "test_util.h"

/* Same traffic density in every direction. */
static SimConfig make_config(int dirs, int initial, int rate,
                             int green, int yellow, int discharge, int ticks)
{
    SimConfig c;

    c.num_directions = dirs;
    c.initial_vehicles = initial;
    sim_config_set_traffic_density(&c, rate);
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
    BOUNDARY(arrival_rate[0], 0, SIM_MAX_ARRIVAL_RATE, "density");
    BOUNDARY(green_ticks, SIM_MIN_GREEN_TICKS, SIM_MAX_GREEN_TICKS, "green");
    BOUNDARY(yellow_ticks, SIM_MIN_YELLOW_TICKS, SIM_MAX_YELLOW_TICKS, "yellow");
    BOUNDARY(discharge_per_tick, SIM_MIN_DISCHARGE, SIM_MAX_DISCHARGE, "per green tick");
    BOUNDARY(total_ticks, SIM_MIN_TICKS, SIM_MAX_TICKS, "simulated ticks");
}

static void test_density_is_validated_for_every_direction(void)
{
    SimConfig c;
    char err[96];
    int d;

    for (d = 0; d < SIM_MAX_DIRECTIONS; d++) {
        sim_config_default(&c);
        c.arrival_rate[d] = SIM_MAX_ARRIVAL_RATE + 1;
        CHECK(sim_config_validate(&c, err, sizeof err) == 0);
        CHECK(strstr(err, "density") != NULL);

        sim_config_default(&c);
        c.arrival_rate[d] = -1;
        CHECK(sim_config_validate(&c, err, sizeof err) == 0);
    }

    sim_config_default(&c);
    c.num_directions = 2;
    c.arrival_rate[3] = 500;                        /* belongs to a direction that is not used */
    CHECK(sim_config_validate(&c, err, sizeof err) == 1);
}

static void test_set_traffic_density_sets_every_direction(void)
{
    SimConfig c;
    int d;

    sim_config_default(&c);
    sim_config_set_traffic_density(&c, 37);
    for (d = 0; d < SIM_MAX_DIRECTIONS; d++) {
        CHECK(c.arrival_rate[d] == 37);
    }
    sim_config_set_traffic_density(NULL, 5);        /* must not crash */
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
     * arrivals (density 0), green 1, yellow 1, one vehicle per green tick, 6 ticks.
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
     * 2 directions, nobody waiting at tick 0, density 50 per 100 ticks in both directions,
     * green 2, yellow 1, one per green tick, 9 ticks.
     * Density 50 builds credit 50, 100: a vehicle arrives at t = 1, 3, 5, 7 in each direction.
     * Signals: t0-1 d0 G | t2 d0 Y | t3-4 d1 G | t5 d1 Y | t6-7 d0 G | t8 d0 Y
     * Arrivals (d0 then d1): t1 ids 1,2 | t3 ids 3,4 | t5 ids 5,6 | t7 ids 7,8
     *   t1: id1 leaves (arrived 1, wait 0)     t3: id2 leaves (arrived 1, wait 2)
     *   t4: id4 leaves (arrived 3, wait 1)     t6: id3 leaves (arrived 3, wait 3)
     *   t7: id5 leaves (arrived 5, wait 2)
     * generated 8, passed 5, dropped 0, remaining 3 (ids 6, 7, 8)
     * total wait 0+2+1+3+2 = 8
     * peak queue 2: e.g. d1 holds ids 2,4 after the t3 arrivals, before id2 leaves.
     */
    SimConfig c = make_config(2, 0, 50, 2, 1, 1, 9);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.generated == 8);
    CHECK(s.passed == 5);
    CHECK(s.dropped == 0);
    CHECK(s.remaining == 3);
    CHECK(s.total_wait == 8);
    CHECK(s.max_queue_length == 2);
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
     * 2 directions, queues pre-filled to capacity (100 each: ids 1-100 in d0, 101-200 in d1),
     * density 100 = one new vehicle per direction every tick, green 1, yellow 1,
     * one per green tick, 3 ticks. Arrivals come before departures in every tick.
     *   t0: id201 -> d0 full, dropped | id202 -> d1 full, dropped. d0 green: id1 leaves (wait 0), d0 = 99.
     *   t1: id203 -> d0 (99 -> 100)   | id204 -> d1 full, dropped. d0 yellow.
     *   t2: id205 -> d0 full, dropped | id206 -> d1 full, dropped. d1 green: id101 leaves (wait 2), d1 = 99.
     * generated 200 + 6 = 206, passed 2, dropped 2 + 1 + 2 = 5, remaining 100 + 99 = 199
     * total wait 0 + 2 = 2, peak queue 100.
     */
    SimConfig c = make_config(2, QUEUE_CAPACITY, 100, 1, 1, 1, 3);
    SimStats s;
    DropProbe p;

    memset(&p, 0, sizeof p);
    CHECK(sim_run(&c, &s, drop_probe, &p) == 1);
    CHECK(s.generated == 206);
    CHECK(s.passed == 2);
    CHECK(s.dropped == 5);
    CHECK(s.remaining == 199);
    CHECK(s.total_wait == 2);
    CHECK(s.max_queue_length == QUEUE_CAPACITY);
    CHECK(stats_is_conserved(&s));
    CHECK(p.dropped_total == 5);                    /* the per-tick view agrees */
    CHECK(p.dropped_at_tick[0] == 2);
    CHECK(p.dropped_at_tick[1] == 1);
    CHECK(p.dropped_at_tick[2] == 2);
}

/* ---- vehicle generation per direction ---- */

typedef struct {
    int prev_len[SIM_MAX_DIRECTIONS];
    int conservation_errors;                        /* queue length != previous + generated - dropped - departed */
    unsigned long generated[SIM_MAX_DIRECTIONS];
    int max_len[SIM_MAX_DIRECTIONS];
} DirProbe;

static void dir_probe_cb(const SimTickInfo *info, void *user)
{
    DirProbe *p = (DirProbe *)user;
    int d;

    for (d = 0; d < info->num_directions; d++) {
        int expected = p->prev_len[d] + info->generated[d]
                     - info->dropped[d] - info->departed[d];

        if (info->queue_length[d] != expected) {
            p->conservation_errors++;
        }
        p->generated[d] += (unsigned long)info->generated[d];
        if (info->queue_length[d] > p->max_len[d]) {
            p->max_len[d] = info->queue_length[d];
        }
        p->prev_len[d] = info->queue_length[d];
    }
}

typedef struct {
    int len0[4];
    int len1[4];
    int gen0[4];
    int gen1[4];
} Trace;

static void trace_cb(const SimTickInfo *info, void *user)
{
    Trace *t = (Trace *)user;

    if (info->tick < 4) {
        t->len0[info->tick] = info->queue_length[0];
        t->len1[info->tick] = info->queue_length[1];
        t->gen0[info->tick] = info->generated[0];
        t->gen1[info->tick] = info->generated[1];
    }
}

static void test_hand_computed_arrival_reaches_only_its_direction(void)
{
    /*
     * 2 directions, nothing waiting at tick 0. Direction 0 has density 0, direction 1 has
     * density 100 (a vehicle every tick). green 1, yellow 1, 2 ticks.
     * Signals: t0 d0 G, t1 d0 Y, so direction 1 stays RED and nothing leaves.
     *   t0: id1 -> d1. d0 empty (green but nobody there). q = [0, 1]
     *   t1: id2 -> d1.                                    q = [0, 2]
     * generated 2, passed 0, remaining 2, peak queue 2.
     */
    SimConfig c = make_config(2, 0, 0, 1, 1, 1, 2);
    SimStats s;
    Trace t;

    c.arrival_rate[1] = 100;
    memset(&t, 0, sizeof t);
    CHECK(sim_run(&c, &s, trace_cb, &t) == 1);

    CHECK(t.gen0[0] == 0 && t.gen0[1] == 0);
    CHECK(t.gen1[0] == 1 && t.gen1[1] == 1);
    CHECK(t.len0[0] == 0 && t.len0[1] == 0);        /* direction 0's queue never changes */
    CHECK(t.len1[0] == 1 && t.len1[1] == 2);        /* direction 1's length rises with each arrival */
    CHECK(s.generated == 2);
    CHECK(s.passed == 0);
    CHECK(s.dropped == 0);
    CHECK(s.remaining == 2);
    CHECK(s.max_queue_length == 2);
    CHECK(stats_is_conserved(&s));
}

static void test_each_direction_gets_its_own_density_without_mixing(void)
{
    /*
     * 4 directions with different densities {100, 50, 20, 0}, 5 vehicles waiting in each
     * at the start, 200 ticks, heavy enough to overflow direction 0.
     * Per direction: generated = 5 + floor(200 * density / 100) = 205, 105, 45, 5.
     * If any vehicle landed in the wrong queue, the per-direction accounting
     * (new length = old length + generated - dropped - departed) would break.
     */
    SimConfig c = make_config(4, 5, 0, 3, 1, 2, 200);
    SimStats s;
    DirProbe p;

    c.arrival_rate[0] = 100;
    c.arrival_rate[1] = 50;
    c.arrival_rate[2] = 20;
    c.arrival_rate[3] = 0;
    memset(&p, 0, sizeof p);

    CHECK(sim_run(&c, &s, dir_probe_cb, &p) == 1);
    CHECK(p.conservation_errors == 0);
    CHECK(p.generated[0] == 205);
    CHECK(p.generated[1] == 105);
    CHECK(p.generated[2] == 45);
    CHECK(p.generated[3] == 5);
    CHECK(s.generated == 205 + 105 + 45 + 5);
    CHECK(s.dropped > 0);                           /* direction 0 overflowed */
    CHECK(p.max_len[0] == QUEUE_CAPACITY);
    CHECK(p.max_len[3] <= 5);                       /* the empty-density direction never grows */
    CHECK(stats_is_conserved(&s));
}

#define ID_LIMIT 4000

typedef struct {
    unsigned char seen[ID_LIMIT];
    int duplicates;
    int bad_ids;                    /* zero, or beyond what was generated */
    int wrong_direction;            /* a vehicle left from a queue it was not generated for */
    int check_blocks;               /* if set: direction d owns IDs d*block+1 .. (d+1)*block */
    unsigned int block;
    unsigned long departed;
} IdProbe;

static void id_probe_cb(const SimTickInfo *info, void *user)
{
    IdProbe *p = (IdProbe *)user;
    int d;
    int k;

    for (d = 0; d < info->num_directions; d++) {
        for (k = 0; k < info->departed[d]; k++) {
            unsigned int id = info->departed_id[d][k];

            p->departed++;
            if (id == 0 || id >= ID_LIMIT) {
                p->bad_ids++;
                continue;
            }
            if (p->seen[id]) {
                p->duplicates++;
            }
            p->seen[id] = 1;
            if (p->check_blocks &&
                (id <= (unsigned int)d * p->block || id > ((unsigned int)d + 1) * p->block)) {
                p->wrong_direction++;
            }
        }
    }
}

static void test_vehicles_leave_from_the_queue_they_were_generated_for(void)
{
    /*
     * 4 directions, 6 initial vehicles each, no later arrivals, green long enough (8) to
     * drain a queue completely, one full 36-tick cycle. The initial vehicles are created
     * direction by direction, so direction d owns exactly the IDs 6d+1 .. 6d+6.
     * All 24 must leave, each ID once, each from its own direction.
     */
    SimConfig c = make_config(4, 6, 0, 8, 1, 1, 36);
    SimStats s;
    IdProbe p;

    memset(&p, 0, sizeof p);
    p.check_blocks = 1;
    p.block = 6;
    CHECK(sim_run(&c, &s, id_probe_cb, &p) == 1);
    CHECK(p.departed == 24);
    CHECK(s.passed == 24);
    CHECK(p.duplicates == 0);
    CHECK(p.bad_ids == 0);
    CHECK(p.wrong_direction == 0);
    CHECK(s.remaining == 0);
}

static void test_vehicle_ids_are_unique_across_a_whole_run(void)
{
    /* Initial vehicles plus ongoing arrivals in every direction, fast discharge so
     * most vehicles leave and their IDs can be observed. */
    SimConfig c = make_config(4, 10, 30, 6, 1, 10, 400);
    SimStats s;
    IdProbe p;
    unsigned int id;
    unsigned long seen_count = 0;

    memset(&p, 0, sizeof p);
    CHECK(sim_run(&c, &s, id_probe_cb, &p) == 1);
    CHECK(p.departed == s.passed);
    CHECK(s.passed > 300);                          /* a meaningful sample */
    CHECK(p.duplicates == 0);                       /* no ID is ever reused */
    CHECK(p.bad_ids == 0);
    for (id = 1; id < ID_LIMIT; id++) {
        if (p.seen[id]) {
            CHECK(id <= s.generated);               /* IDs run 1..generated */
            seen_count++;
        }
    }
    CHECK(seen_count == s.passed);
}

static void test_direction_with_zero_density_stays_empty(void)
{
    SimConfig c = make_config(4, 0, 0, 3, 1, 1, 80);
    SimStats s;
    DirProbe p;
    int d;

    c.arrival_rate[0] = 100;                        /* only North receives traffic */
    memset(&p, 0, sizeof p);

    CHECK(sim_run(&c, &s, dir_probe_cb, &p) == 1);
    CHECK(p.conservation_errors == 0);
    CHECK(p.generated[0] == 80);
    for (d = 1; d < 4; d++) {
        CHECK(p.generated[d] == 0);
        CHECK(p.max_len[d] == 0);                   /* no vehicle ever appears in these queues */
    }
    CHECK(s.generated == 80);
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
    SimConfig c = make_config(4, 30, 33, 4, 1, 1, 150);
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
    static const int density[] = { 0, 100, 33, 14 };
    static const int green[] = { 1, 3 };
    static const int discharge[] = { 1, 3 };
    size_t a, b, c, d, e;
    int runs = 0;

    for (a = 0; a < sizeof dirs / sizeof dirs[0]; a++)
    for (b = 0; b < sizeof initial / sizeof initial[0]; b++)
    for (c = 0; c < sizeof density / sizeof density[0]; c++)
    for (d = 0; d < sizeof green / sizeof green[0]; d++)
    for (e = 0; e < sizeof discharge / sizeof discharge[0]; e++) {
        SimConfig cfg = make_config(dirs[a], initial[b], density[c],
                                    green[d], 1, discharge[e], 200);
        SimStats s;
        /* every direction generates its initial vehicles plus floor(200 * density / 100) */
        unsigned long expected = (unsigned long)dirs[a] *
            ((unsigned long)initial[b] + (unsigned long)(200 * density[c] / 100));

        CHECK(sim_run(&cfg, &s, NULL, NULL) == 1);
        CHECK(s.generated == expected);             /* the density decides how much traffic arrives */
        CHECK(stats_is_conserved(&s));              /* nothing lost, nothing duplicated */
        CHECK(s.max_queue_length <= QUEUE_CAPACITY);
        CHECK(s.passed <= s.generated);
        runs++;
    }
    CHECK(runs == 3 * 4 * 4 * 2 * 2);
}

static void test_higher_density_means_more_traffic(void)
{
    static const int densities[] = { 0, 10, 40, 100 };
    unsigned long previous = 0;
    size_t i;

    for (i = 0; i < sizeof densities / sizeof densities[0]; i++) {
        SimConfig c = make_config(4, 5, densities[i], 5, 1, 1, 100);
        SimStats s;

        CHECK(sim_run(&c, &s, NULL, NULL) == 1);
        if (i > 0) {
            CHECK(s.generated > previous);
        }
        previous = s.generated;
    }
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
    SimConfig c = make_config(4, QUEUE_CAPACITY, 100, 1, 1, 1, 400);
    SimStats s;

    CHECK(sim_run(&c, &s, NULL, NULL) == 1);
    CHECK(s.dropped > 0);
    CHECK(s.max_queue_length == QUEUE_CAPACITY);
    CHECK(stats_is_conserved(&s));
}

/* ---- robustness and repeatability ---- */

static void test_invalid_arguments_are_rejected(void)
{
    SimConfig c = make_config(4, 10, 20, 5, 1, 1, 60);
    SimStats s;

    CHECK(sim_run(NULL, &s, NULL, NULL) == 0);
    CHECK(sim_run(&c, NULL, NULL, NULL) == 0);

    c.num_directions = 1;
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, QUEUE_CAPACITY + 1, 20, 5, 1, 1, 60);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, 10, 20, 0, 1, 1, 60);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, 10, 20, 5, 1, 1, 0);
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c = make_config(4, 10, 20, 5, 1, 1, 60);
    c.arrival_rate[2] = SIM_MAX_ARRIVAL_RATE + 1;   /* one bad direction rejects the whole run */
    CHECK(sim_run(&c, &s, NULL, NULL) == 0);
    c.arrival_rate[2] = -1;
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
    TEST_RUN(test_density_is_validated_for_every_direction);
    TEST_RUN(test_set_traffic_density_sets_every_direction);
    TEST_RUN(test_config_validate_edge_cases);
    TEST_RUN(test_signal_sequence_for_two_directions);
    TEST_RUN(test_one_direction_active_at_a_time_and_all_served);
    TEST_RUN(test_signal_invalid_arguments_are_red);
    TEST_RUN(test_hand_computed_initial_vehicles_only);
    TEST_RUN(test_hand_computed_with_ongoing_arrivals);
    TEST_RUN(test_hand_computed_multiple_departures_per_green_tick);
    TEST_RUN(test_hand_computed_overflow_drops_and_counts);
    TEST_RUN(test_hand_computed_arrival_reaches_only_its_direction);
    TEST_RUN(test_each_direction_gets_its_own_density_without_mixing);
    TEST_RUN(test_direction_with_zero_density_stays_empty);
    TEST_RUN(test_vehicles_leave_from_the_queue_they_were_generated_for);
    TEST_RUN(test_vehicle_ids_are_unique_across_a_whole_run);
    TEST_RUN(test_only_green_direction_moves_and_others_are_unchanged);
    TEST_RUN(test_report_matches_what_was_observed);
    TEST_RUN(test_conservation_across_parameter_sweep);
    TEST_RUN(test_higher_density_means_more_traffic);
    TEST_RUN(test_light_traffic_never_drops);
    TEST_RUN(test_heavy_traffic_drops_but_stays_consistent);
    TEST_RUN(test_invalid_arguments_are_rejected);
    TEST_RUN(test_runs_are_repeatable);
    TEST_RUN(test_default_run_report_is_consistent);
    return TEST_SUMMARY();
}
