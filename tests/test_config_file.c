#include "config_file.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

#define CONFIG_TEST_PATH "tms_test_config.csv"
#define CONFIG_TEST_BAD_PATH "missing_config_directory/config.csv"

static const char header[] =
    "directions,north_density,south_density,east_density,west_density,"
    "green_ticks,yellow_ticks,queue_capacity,initial_vehicles,total_ticks";

static SimConfig base_config(void)
{
    SimConfig config;
    sim_config_default(&config);
    config.num_directions = 4;
    config.arrival_rate[0] = 11;
    config.arrival_rate[1] = 22;
    config.arrival_rate[2] = 33;
    config.arrival_rate[3] = 44;
    config.green_ticks = 7;
    config.yellow_ticks = 3;
    config.queue_capacity = 31;
    config.initial_vehicles = 9;
    config.total_ticks = 987;
    return config;
}

static int same_config(const SimConfig *a, const SimConfig *b)
{
    int d;
    if (a->num_directions != b->num_directions ||
        a->initial_vehicles != b->initial_vehicles ||
        a->queue_capacity != b->queue_capacity ||
        a->green_ticks != b->green_ticks ||
        a->yellow_ticks != b->yellow_ticks ||
        a->total_ticks != b->total_ticks ||
        a->discharge_per_tick != b->discharge_per_tick) return 0;
    for (d = 0; d < SIM_MAX_DIRECTIONS; d++)
        if (a->arrival_rate[d] != b->arrival_rate[d]) return 0;
    return 1;
}

static int write_text(const char *text)
{
    FILE *file = fopen(CONFIG_TEST_PATH, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fputs(text, file) >= 0;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int reject_text(const char *text)
{
    SimConfig current = base_config();
    SimConfig original = current;
    char error[160];
    int ok;
    if (!write_text(text)) return 0;
    ok = !config_csv_load(CONFIG_TEST_PATH, &current, error, sizeof error) &&
         error[0] != '\0' && same_config(&current, &original);
    remove(CONFIG_TEST_PATH);
    return ok;
}

static void test_round_trip_all_directional_densities(void)
{
    SimConfig expected = base_config();
    SimConfig loaded = base_config();
    char error[160];

    CHECK(config_csv_save(CONFIG_TEST_PATH, &expected, error, sizeof error) == 1);
    CHECK(config_csv_load(CONFIG_TEST_PATH, &loaded, error, sizeof error) == 1);
    CHECK(same_config(&loaded, &expected));
    remove(CONFIG_TEST_PATH);
}

static void test_inactive_directions_are_empty_and_round_trip(void)
{
    SimConfig expected = base_config();
    SimConfig loaded = base_config();
    char error[160];

    expected.num_directions = 2;
    expected.arrival_rate[2] = 0;
    expected.arrival_rate[3] = 0;
    CHECK(config_csv_save(CONFIG_TEST_PATH, &expected, error, sizeof error) == 1);
    CHECK(config_csv_load(CONFIG_TEST_PATH, &loaded, error, sizeof error) == 1);
    CHECK(same_config(&loaded, &expected));
    remove(CONFIG_TEST_PATH);
}

static void test_rejected_files_preserve_current_config(void)
{
    char file[1200];

    CHECK(reject_text(""));
    CHECK(reject_text("1,2,3\n")); /* missing required header and row */
    {
        char text[400];
        snprintf(text, sizeof text, "%s\n3,11,22\n", header);
        CHECK(reject_text(text)); /* truncated row */
    }
    {
        char text[400];
        snprintf(text, sizeof text,
                 "directions,north_density,unknown,south_density,east_density,west_density,green_ticks,yellow_ticks,queue_capacity,initial_vehicles,total_ticks\n"
                 "4,11,22,33,44,55,7,3,31,9,987\n");
        CHECK(reject_text(text)); /* unknown and duplicate/misordered schema */
    }
    {
        char text[400];
        snprintf(text, sizeof text,
                 "directions,north_density,north_density,east_density,west_density,green_ticks,yellow_ticks,queue_capacity,initial_vehicles,total_ticks\n"
                 "4,11,22,33,44,7,3,31,9,987\n");
        CHECK(reject_text(text)); /* duplicate field */
    }
    {
        char text[500];
        snprintf(text, sizeof text, "%s\n2,11,22,33,,7,3,31,9,987\n", header);
        CHECK(reject_text(text)); /* inactive direction must be empty */
        snprintf(text, sizeof text, "%s\n2,,22,,,7,3,31,9,987\n", header);
        CHECK(reject_text(text)); /* active density cannot be empty */
        snprintf(text, sizeof text, "%s\n4,101,22,33,44,7,3,31,9,987\n", header);
        CHECK(reject_text(text)); /* out-of-range density */
        snprintf(text, sizeof text, "%s\n4,eleven,22,33,44,7,3,31,9,987\n", header);
        CHECK(reject_text(text)); /* corrupted number */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,3,8,9,987\n", header);
        CHECK(reject_text(text)); /* initial vehicles exceed capacity */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,3,0,0,987\n", header);
        CHECK(reject_text(text)); /* invalid capacity */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,3,31,9,0\n", header);
        CHECK(reject_text(text)); /* invalid run length */
        snprintf(text, sizeof text, "%s\n1,11,,,,7,3,31,0,987\n", header);
        CHECK(reject_text(text)); /* invalid direction count */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,3,31,9,987,extra\n", header);
        CHECK(reject_text(text)); /* extra/unknown value */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,3,31,9,987\nextra\n", header);
        CHECK(reject_text(text)); /* multiple data rows */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,0,3,31,9,987\n", header);
        CHECK(reject_text(text)); /* green below range */
        snprintf(text, sizeof text, "%s\n4,11,22,33,44,7,31,31,9,987\n", header);
        CHECK(reject_text(text)); /* yellow above range */
    }
    snprintf(file, sizeof file, "%s\n%0600d\n", header, 1);
    CHECK(reject_text(file)); /* oversized data row */
    {
        char oversized_header[CONFIG_CSV_MAX_LINE + 20];
        memset(oversized_header, 'H', sizeof oversized_header - 1);
        oversized_header[sizeof oversized_header - 1] = '\0';
        CHECK(reject_text(oversized_header));
    }
}

static void test_save_replaces_existing_file_and_reports_failure(void)
{
    SimConfig config = base_config();
    SimConfig loaded = base_config();
    char error[160];

    CHECK(config_csv_save(CONFIG_TEST_PATH, &config, error, sizeof error) == 1);
#ifndef _WIN32
    {
        struct stat info;
        CHECK(stat(CONFIG_TEST_PATH, &info) == 0);
        CHECK((info.st_mode & 0777) == 0600);
        if (geteuid() != 0) {
            CHECK(chmod(CONFIG_TEST_PATH, 0444) == 0);
            CHECK(config_csv_save(CONFIG_TEST_PATH, &config, error, sizeof error) == 0);
            CHECK(chmod(CONFIG_TEST_PATH, 0600) == 0);
        }
    }
#endif
    config.arrival_rate[0] = 77;
    CHECK(config_csv_save(CONFIG_TEST_PATH, &config, error, sizeof error) == 1);
    CHECK(config_csv_load(CONFIG_TEST_PATH, &loaded, error, sizeof error) == 1);
    CHECK(loaded.arrival_rate[0] == 77);
    CHECK(config_csv_save(CONFIG_TEST_BAD_PATH, &config, error, sizeof error) == 0);
    CHECK(error[0] != '\0');
    {
        SimConfig original = loaded;
        CHECK(config_csv_load(CONFIG_TEST_BAD_PATH, &loaded, error, sizeof error) == 0);
        CHECK(same_config(&loaded, &original));
    }
    remove(CONFIG_TEST_PATH);
}

int main(void)
{
    printf("test_config_file\n");
    TEST_RUN(test_round_trip_all_directional_densities);
    TEST_RUN(test_inactive_directions_are_empty_and_round_trip);
    TEST_RUN(test_rejected_files_preserve_current_config);
    TEST_RUN(test_save_replaces_existing_file_and_reports_failure);
    return TEST_SUMMARY();
}
