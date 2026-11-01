/*
 * Unit tests: VehicleGenerator (F-001, F-002, TC-Gen-01, TC-Gen-02).
 * Density follows the configured rate exactly, IDs are unique across all
 * directions, and directions never influence each other.
 */
#include <stdio.h>
#include <string.h>

#include "test_util.h"
#include "vehicle_generator.h"

#define MAX_IDS 4000

static void test_init_validation(void)
{
    VehicleGenerator vg;
    int ok[4] = { 0, 50, 100, 7 };
    int low[4] = { 10, -1, 10, 10 };
    int high[4] = { 10, 10, 101, 10 };

    CHECK(vg_init(&vg, 4, ok) == 1);
    CHECK(vg_init(&vg, 1, ok) == 1);
    CHECK(vg_init(NULL, 4, ok) == 0);
    CHECK(vg_init(&vg, 4, NULL) == 0);
    CHECK(vg_init(&vg, 0, ok) == 0);
    CHECK(vg_init(&vg, -1, ok) == 0);
    CHECK(vg_init(&vg, VG_MAX_DIRECTIONS + 1, ok) == 0);
    CHECK(vg_init(&vg, 4, low) == 0);                /* -1 */
    CHECK(vg_init(&vg, 4, high) == 0);               /* 101 = max + 1 */
    CHECK(vg_init(&vg, 2, high) == 1);               /* bad entry belongs to an unused direction */
}

static void test_rate_zero_never_generates(void)
{
    VehicleGenerator vg;
    int rates[1] = { 0 };
    Vehicle v;
    unsigned int t;
    int count = 0;

    CHECK(vg_init(&vg, 1, rates) == 1);
    for (t = 0; t < 1000; t++) {
        count += vg_arrival(&vg, 0, t, &v);
    }
    CHECK(count == 0);
}

static void test_rate_hundred_generates_every_tick(void)
{
    VehicleGenerator vg;
    int rates[1] = { 100 };
    Vehicle v;
    unsigned int t;

    CHECK(vg_init(&vg, 1, rates) == 1);
    for (t = 0; t < 300; t++) {
        CHECK(vg_arrival(&vg, 0, t, &v) == 1);
    }
}

static void test_arrivals_follow_density_exactly(void)
{
    static const int rates_to_test[] = { 1, 7, 20, 33, 50, 99 };
    size_t i;

    for (i = 0; i < sizeof rates_to_test / sizeof rates_to_test[0]; i++) {
        VehicleGenerator vg;
        int rate[1];
        Vehicle v;
        unsigned int t;
        unsigned int count = 0;

        rate[0] = rates_to_test[i];
        CHECK(vg_init(&vg, 1, rate) == 1);
        for (t = 0; t < 1000; t++) {
            count += (unsigned int)vg_arrival(&vg, 0, t, &v);
            /* after every prefix of N ticks: exactly floor(N * rate / 100) vehicles */
            CHECK(count == (t + 1) * (unsigned int)rate[0] / 100);
            if (t == 99) {
                CHECK(count == (unsigned int)rate[0]);   /* rate = vehicles per 100 ticks */
            }
        }
        CHECK(count == (unsigned int)rate[0] * 10);
    }
}

static void test_rate_twenty_arrives_every_fifth_tick(void)
{
    static const unsigned int expected[] = { 4, 9, 14, 19 };
    VehicleGenerator vg;
    int rate[1] = { 20 };
    Vehicle v;
    unsigned int t;
    size_t found = 0;

    CHECK(vg_init(&vg, 1, rate) == 1);
    for (t = 0; t < 20; t++) {
        if (vg_arrival(&vg, 0, t, &v)) {
            CHECK(found < 4 && t == expected[found]);
            CHECK(v.arrival_tick == t);                  /* timestamp recorded */
            found++;
        }
    }
    CHECK(found == 4);
}

static void mark_id(unsigned char *seen, unsigned long *total, Vehicle v)
{
    CHECK(v.id >= 1 && v.id < MAX_IDS);
    if (v.id >= 1 && v.id < MAX_IDS) {
        CHECK(seen[v.id] == 0);                          /* never handed out twice */
        seen[v.id] = 1;
    }
    (*total)++;
}

static void test_ids_unique_across_directions_and_initial_batch(void)
{
    static unsigned char seen[MAX_IDS];
    VehicleGenerator vg;
    int rates[4] = { 100, 50, 20, 7 };
    unsigned long total = 0;
    unsigned long id;
    unsigned int t;
    unsigned int k;
    int d;
    Vehicle v;

    memset(seen, 0, sizeof seen);
    CHECK(vg_init(&vg, 4, rates) == 1);

    for (d = 0; d < 4; d++) {
        for (k = 0; k < 3; k++) {
            mark_id(seen, &total, vg_new_vehicle(&vg, 0));
        }
    }
    for (t = 0; t < 500; t++) {
        for (d = 0; d < 4; d++) {
            if (vg_arrival(&vg, d, t, &v)) {
                mark_id(seen, &total, v);
            }
        }
    }

    CHECK(total == 12 + 500 + 250 + 100 + 35);
    for (id = 1; id <= total; id++) {
        CHECK(seen[id] == 1);                            /* IDs run 1..N with no gaps */
    }
    CHECK(seen[total + 1] == 0);
}

static void test_each_direction_has_its_own_density(void)
{
    VehicleGenerator vg;
    int rates[4] = { 100, 50, 20, 0 };
    unsigned int counts[4] = { 0, 0, 0, 0 };
    unsigned int t;
    int d;
    Vehicle v;

    CHECK(vg_init(&vg, 4, rates) == 1);
    for (t = 0; t < 200; t++) {
        for (d = 0; d < 4; d++) {
            counts[d] += (unsigned int)vg_arrival(&vg, d, t, &v);
        }
    }
    CHECK(counts[0] == 200);     /* North: 100 per 100 ticks */
    CHECK(counts[1] == 100);     /* South:  50 per 100 ticks */
    CHECK(counts[2] == 40);      /* East:   20 per 100 ticks */
    CHECK(counts[3] == 0);       /* West:    0 */
}

static void test_directions_do_not_influence_each_other(void)
{
    VehicleGenerator vg;
    int rates[2] = { 50, 50 };
    Vehicle v;
    unsigned int t;
    int arrivals0 = 0;

    CHECK(vg_init(&vg, 2, rates) == 1);
    for (t = 0; t < 10; t++) {
        arrivals0 += vg_arrival(&vg, 0, t, &v);          /* only direction 0 is ticked */
    }
    CHECK(arrivals0 == 5);
    /* direction 1 starts from scratch: credit 50 first, so no vehicle yet, then one */
    CHECK(vg_arrival(&vg, 1, 10, &v) == 0);
    CHECK(vg_arrival(&vg, 1, 11, &v) == 1);
    CHECK(v.arrival_tick == 11);
}

static void test_new_vehicle_records_tick_and_increments_id(void)
{
    VehicleGenerator vg;
    int rates[2] = { 0, 0 };
    Vehicle a;
    Vehicle b;

    CHECK(vg_init(&vg, 2, rates) == 1);
    a = vg_new_vehicle(&vg, 7);
    b = vg_new_vehicle(&vg, 9);
    CHECK(a.id == 1 && a.arrival_tick == 7);
    CHECK(b.id == 2 && b.arrival_tick == 9);
    CHECK(vg_new_vehicle(NULL, 5).id == 0);              /* 0 marks "no vehicle" */
}

static void test_invalid_arguments_are_safe(void)
{
    VehicleGenerator vg;
    int rates[2] = { 100, 100 };
    Vehicle v;

    v.id = 77;
    v.arrival_tick = 88;
    CHECK(vg_init(&vg, 2, rates) == 1);
    CHECK(vg_arrival(NULL, 0, 0, &v) == 0);
    CHECK(vg_arrival(&vg, 0, 0, NULL) == 0);
    CHECK(vg_arrival(&vg, -1, 0, &v) == 0);
    CHECK(vg_arrival(&vg, 2, 0, &v) == 0);               /* one past the last direction */
    CHECK(v.id == 77 && v.arrival_tick == 88);           /* untouched on failure */
    CHECK(vg_arrival(&vg, 0, 0, &v) == 1 && v.id == 1);  /* failed calls consumed no ID */
}

int main(void)
{
    printf("test_vehicle_generator\n");
    TEST_RUN(test_init_validation);
    TEST_RUN(test_rate_zero_never_generates);
    TEST_RUN(test_rate_hundred_generates_every_tick);
    TEST_RUN(test_arrivals_follow_density_exactly);
    TEST_RUN(test_rate_twenty_arrives_every_fifth_tick);
    TEST_RUN(test_ids_unique_across_directions_and_initial_batch);
    TEST_RUN(test_each_direction_has_its_own_density);
    TEST_RUN(test_directions_do_not_influence_each_other);
    TEST_RUN(test_new_vehicle_records_tick_and_increments_id);
    TEST_RUN(test_invalid_arguments_are_safe);
    return TEST_SUMMARY();
}
