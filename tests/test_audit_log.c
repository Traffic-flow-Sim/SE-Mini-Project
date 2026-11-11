
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audit_log.h"
#include "test_util.h"

#define TEST_LOG_FILE "test_audit_log.tmp"

static int file_contains(const char *path, const char *text)
{
    char line[512];
    FILE *file = fopen(path, "r");
    int found = 0;

    if (file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        if (strstr(line, text) != NULL) {
            found = 1;
            break;
        }
    }

    fclose(file);
    return found;
}

static int count_log_lines(const char *path)
{
    char line[512];
    FILE *file = fopen(path, "r");
    int count = 0;

    if (file == NULL) {
        return -1;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        count++;
    }

    fclose(file);
    return count;
}

static void test_signal_event_is_persisted(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 12, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 1);

    CHECK(file_contains(TEST_LOG_FILE, "SIGNAL"));
    CHECK(file_contains(TEST_LOG_FILE, "Intersection=1"));
    CHECK(file_contains(TEST_LOG_FILE, "Tick=12"));
    CHECK(file_contains(TEST_LOG_FILE, "Direction=North"));
    CHECK(file_contains(TEST_LOG_FILE, "Previous=RED"));
    CHECK(file_contains(TEST_LOG_FILE, "New=GREEN"));

    /* Timestamp format: YYYY-MM-DD HH:MM:SS */
    {
        FILE *file = fopen(TEST_LOG_FILE, "r");
        char line[512];

        CHECK(file != NULL);

        if (file != NULL) {
            CHECK(fgets(line, sizeof line, file) != NULL);
            CHECK(strlen(line) >= 19);
            CHECK(line[4] == '-');
            CHECK(line[7] == '-');
            CHECK(line[10] == ' ');
            CHECK(line[13] == ':');
            CHECK(line[16] == ':');
            fclose(file);
        }
    }

    remove(TEST_LOG_FILE);
}

static void test_events_are_appended(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 0, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 1);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 2, 5, 1,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 1);

    CHECK(count_log_lines(TEST_LOG_FILE) == 2);
    CHECK(file_contains(TEST_LOG_FILE, "Intersection=1"));
    CHECK(file_contains(TEST_LOG_FILE, "Intersection=2"));

    remove(TEST_LOG_FILE);
}

static void test_configuration_event_is_persisted(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_config_change(
        TEST_LOG_FILE,
        2,
        "Green duration changed") == 1);

    CHECK(file_contains(TEST_LOG_FILE, "CONFIG"));
    CHECK(file_contains(TEST_LOG_FILE, "Intersection=2"));
    CHECK(file_contains(TEST_LOG_FILE, "Green duration changed"));

    remove(TEST_LOG_FILE);
}

static void test_configuration_description_is_sanitized(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_config_change(
        TEST_LOG_FILE,
        1,
        "Density changed\nInjected entry|extra") == 1);

    CHECK(count_log_lines(TEST_LOG_FILE) == 1);
    CHECK(file_contains(
        TEST_LOG_FILE,
        "Density changed Injected entry extra"));

    remove(TEST_LOG_FILE);
}

static void test_invalid_signal_events_are_rejected(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_signal(
        NULL, 1, 0, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 0, 0, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, -1, 0, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 0, -1,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 0, SIM_MAX_DIRECTIONS,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 0, 0,
        SIM_SIGNAL_GREEN, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_signal(
        TEST_LOG_FILE, 1, 0, 0,
        (SimSignal)99, SIM_SIGNAL_GREEN) == 0);

    CHECK(count_log_lines(TEST_LOG_FILE) == -1);

    remove(TEST_LOG_FILE);
}

static void test_invalid_configuration_events_are_rejected(void)
{
    remove(TEST_LOG_FILE);

    CHECK(audit_log_config_change(
        TEST_LOG_FILE, 0, "Invalid ID") == 0);

    CHECK(audit_log_config_change(
        TEST_LOG_FILE, 1, NULL) == 0);

    CHECK(audit_log_config_change(
        TEST_LOG_FILE, 1, "") == 0);

    CHECK(count_log_lines(TEST_LOG_FILE) == -1);

    remove(TEST_LOG_FILE);
}

static void test_file_open_failure_is_reported(void)
{
    CHECK(audit_log_signal(
        "missing_audit_directory/test.log",
        1, 0, 0,
        SIM_SIGNAL_RED, SIM_SIGNAL_GREEN) == 0);

    CHECK(audit_log_config_change(
        "missing_audit_directory/test.log",
        1,
        "Configuration changed") == 0);
}

int main(void)
{
    TEST_RUN(test_signal_event_is_persisted);
    TEST_RUN(test_events_are_appended);
    TEST_RUN(test_configuration_event_is_persisted);
    TEST_RUN(test_configuration_description_is_sanitized);
    TEST_RUN(test_invalid_signal_events_are_rejected);
    TEST_RUN(test_invalid_configuration_events_are_rejected);
    TEST_RUN(test_file_open_failure_is_reported);

    remove(TEST_LOG_FILE);

    return TEST_SUMMARY();
}
