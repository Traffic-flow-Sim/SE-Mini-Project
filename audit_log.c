
#include <stdio.h>
#include <time.h>

#include "audit_log.h"

static const char *signal_text(SimSignal signal)
{
    switch (signal) {
        case SIM_SIGNAL_RED:
            return "RED";
        case SIM_SIGNAL_GREEN:
            return "GREEN";
        case SIM_SIGNAL_YELLOW:
            return "YELLOW";
        default:
            return NULL;
    }
}

static int create_timestamp(char *buffer, size_t buffer_size)
{
    time_t current_time;
    struct tm *local_time;

    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }

    current_time = time(NULL);

    if (current_time == (time_t)-1) {
        return 0;
    }

    local_time = localtime(&current_time);

    if (local_time == NULL) {
        return 0;
    }

    return strftime(
        buffer,
        buffer_size,
        "%Y-%m-%d %H:%M:%S",
        local_time) > 0;
}

static int open_audit_file(const char *path, FILE **file)
{
    if (path == NULL || path[0] == '\0' || file == NULL) {
        return 0;
    }

    *file = fopen(path, "a");

    return *file != NULL;
}

static int finish_audit_write(FILE *file, int write_ok)
{
    int flush_ok;
    int close_ok;

    if (file == NULL) {
        return 0;
    }

    flush_ok = fflush(file) == 0;
    close_ok = fclose(file) == 0;

    return write_ok && flush_ok && close_ok;
}

int audit_log_signal(
    const char *path,
    int intersection_id,
    unsigned int tick,
    int direction,
    SimSignal previous_signal,
    SimSignal new_signal)
{
    static const char *direction_names[SIM_MAX_DIRECTIONS] = {
        "North", "South", "East", "West"
    };

    char timestamp[32];
    const char *previous_name;
    const char *new_name;
    FILE *file;
    int write_ok;

    if (intersection_id < 1 ||
        direction < 0 ||
        direction >= SIM_MAX_DIRECTIONS ||
        previous_signal == new_signal) {
        return 0;
    }

    previous_name = signal_text(previous_signal);
    new_name = signal_text(new_signal);

    if (previous_name == NULL || new_name == NULL) {
        return 0;
    }

    if (!create_timestamp(timestamp, sizeof timestamp)) {
        return 0;
    }

    if (!open_audit_file(path, &file)) {
        return 0;
    }

    write_ok = fprintf(
        file,
        "%s | SIGNAL | Intersection=%d | Tick=%u | "
        "Direction=%s | Previous=%s | New=%s\n",
        timestamp,
        intersection_id,
        tick,
        direction_names[direction],
        previous_name,
        new_name) >= 0;

    return finish_audit_write(file, write_ok);
}

int audit_log_config_change(
    const char *path,
    int intersection_id,
    const char *description)
{
    char timestamp[32];
    const char *cursor;
    FILE *file;
    int write_ok;

    if (intersection_id < 1 ||
        description == NULL ||
        description[0] == '\0') {
        return 0;
    }

    if (!create_timestamp(timestamp, sizeof timestamp)) {
        return 0;
    }

    if (!open_audit_file(path, &file)) {
        return 0;
    }

    write_ok = fprintf(
        file,
        "%s | CONFIG | Intersection=%d | Change=",
        timestamp,
        intersection_id) >= 0;

    cursor = description;

    while (write_ok && *cursor != '\0') {
        char character = *cursor++;

        if (character == '\n' ||
            character == '\r' ||
            character == '\t' ||
            character == '|') {
            character = ' ';
        }

        if (fputc((unsigned char)character, file) == EOF) {
            write_ok = 0;
        }
    }

    if (write_ok && fputc('\n', file) == EOF) {
        write_ok = 0;
    }

    return finish_audit_write(file, write_ok);
}
