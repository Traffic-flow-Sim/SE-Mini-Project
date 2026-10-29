/*
 * S1TMS-9: statistics and end-of-run report.
 * Hand-computed averages, peak tracking, the conservation check and the
 * report text.
 */
#include <stdio.h>
#include <string.h>

#include "stats.h"
#include "test_util.h"

static int near(double a, double b)
{
    double d = a - b;

    return d > -1e-9 && d < 1e-9;
}

static void test_init_zeroes_everything(void)
{
    SimStats s;

    memset(&s, 0xAB, sizeof s);
    stats_init(&s);
    CHECK(s.generated == 0 && s.passed == 0 && s.dropped == 0);
    CHECK(s.remaining == 0 && s.total_wait == 0);
    CHECK(s.max_queue_length == 0 && s.ticks == 0);
    stats_init(NULL);                               /* must not crash */
}

static void test_average_wait_hand_computed(void)
{
    SimStats s;

    stats_init(&s);
    stats_record_departure(&s, 2);
    stats_record_departure(&s, 4);
    stats_record_departure(&s, 6);                  /* (2 + 4 + 6) / 3 = 4.0 */
    CHECK(s.passed == 3);
    CHECK(s.total_wait == 12);
    CHECK(near(stats_average_wait(&s), 4.0));

    stats_record_departure(&s, 0);                  /* (12 + 0) / 4 = 3.0 */
    CHECK(near(stats_average_wait(&s), 3.0));
}

static void test_average_wait_without_departures_is_zero(void)
{
    SimStats s;

    stats_init(&s);
    CHECK(near(stats_average_wait(&s), 0.0));       /* no division by zero */
    CHECK(near(stats_average_wait(NULL), 0.0));
}

static void test_max_queue_length_keeps_only_the_peak(void)
{
    SimStats s;

    stats_init(&s);
    stats_record_queue_length(&s, 3);
    stats_record_queue_length(&s, 7);
    stats_record_queue_length(&s, 2);
    stats_record_queue_length(&s, 7);
    CHECK(s.max_queue_length == 7);
    stats_record_queue_length(&s, 8);
    CHECK(s.max_queue_length == 8);
    stats_record_queue_length(NULL, 99);            /* must not crash */
}

static void test_conservation_check(void)
{
    SimStats s;

    stats_init(&s);
    s.generated = 10;
    s.passed = 6;
    s.dropped = 1;
    s.remaining = 3;
    CHECK(stats_is_conserved(&s) == 1);

    s.remaining = 4;                                /* a vehicle appeared from nowhere */
    CHECK(stats_is_conserved(&s) == 0);
    s.remaining = 2;                                /* a vehicle was lost */
    CHECK(stats_is_conserved(&s) == 0);
    CHECK(stats_is_conserved(NULL) == 0);
}

static void test_record_generated_counts(void)
{
    SimStats s;
    int i;

    stats_init(&s);
    for (i = 0; i < 25; i++) {
        stats_record_generated(&s);
    }
    CHECK(s.generated == 25);
    stats_record_generated(NULL);
}

static void test_report_contents(void)
{
    SimStats s;
    char text[1024];
    int needed;

    stats_init(&s);
    s.ticks = 10;
    s.generated = 5;
    s.dropped = 1;
    s.remaining = 2;
    stats_record_departure(&s, 3);
    stats_record_departure(&s, 5);                  /* average wait 4.00 */
    stats_record_queue_length(&s, 6);

    needed = report_format(text, sizeof text, &s);
    CHECK(needed > 0 && needed < (int)sizeof text);
    CHECK(strstr(text, "Ticks simulated       : 10") != NULL);
    CHECK(strstr(text, "Vehicles generated    : 5") != NULL);
    CHECK(strstr(text, "Vehicles processed    : 2") != NULL);
    CHECK(strstr(text, "Vehicles dropped      : 1") != NULL);
    CHECK(strstr(text, "Vehicles still queued : 2") != NULL);
    CHECK(strstr(text, "Average wait (ticks)  : 4.00") != NULL);
    CHECK(strstr(text, "Maximum queue length  : 6") != NULL);
    CHECK(strstr(text, "Integrity check       : OK") != NULL);

    s.remaining = 3;                                /* break the invariant */
    CHECK(report_format(text, sizeof text, &s) > 0);
    CHECK(strstr(text, "Integrity check       : FAILED") != NULL);
}

static void test_report_format_is_buffer_safe(void)
{
    SimStats s;
    char small[16];
    char guard[8];
    int needed;

    stats_init(&s);
    memset(guard, 'G', sizeof guard);

    needed = report_format(small, sizeof small, &s);
    CHECK(needed > (int)sizeof small);              /* reports the full length needed */
    CHECK(strlen(small) < sizeof small);            /* truncated but terminated */
    CHECK(report_format(NULL, 0, &s) == needed);    /* size query */
    CHECK(report_format(NULL, 5, &s) == -1);        /* NULL buffer with a size */
    CHECK(report_format(small, sizeof small, NULL) == -1);
    CHECK(guard[0] == 'G' && guard[7] == 'G');
}

static void test_report_print_writes_text(void)
{
    SimStats s;
    char line[128];
    FILE *f;

    stats_init(&s);
    s.ticks = 4;
    f = fopen("tms_test_report.tmp", "w+b");
    CHECK(f != NULL);
    if (f == NULL) {
        return;
    }
    report_print(f, &s);
    report_print(NULL, &s);                         /* must not crash */
    report_print(f, NULL);
    rewind(f);
    CHECK(fgets(line, sizeof line, f) != NULL);
    CHECK(strstr(line, "SIMULATION SUMMARY") != NULL);
    fclose(f);
    remove("tms_test_report.tmp");
}

int main(void)
{
    printf("test_stats\n");
    TEST_RUN(test_init_zeroes_everything);
    TEST_RUN(test_average_wait_hand_computed);
    TEST_RUN(test_average_wait_without_departures_is_zero);
    TEST_RUN(test_max_queue_length_keeps_only_the_peak);
    TEST_RUN(test_conservation_check);
    TEST_RUN(test_record_generated_counts);
    TEST_RUN(test_report_contents);
    TEST_RUN(test_report_format_is_buffer_safe);
    TEST_RUN(test_report_print_writes_text);
    return TEST_SUMMARY();
}
