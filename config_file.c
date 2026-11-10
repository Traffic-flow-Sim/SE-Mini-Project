#include "config_file.h"

#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "input.h"

static const char csv_header[] =
    "directions,north_density,south_density,east_density,west_density,"
    "green_ticks,yellow_ticks,queue_capacity,initial_vehicles,total_ticks";

static const char *const field_names[] = {
    "directions", "north_density", "south_density", "east_density",
    "west_density", "green_ticks", "yellow_ticks", "queue_capacity",
    "initial_vehicles", "total_ticks"
};

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0)
        snprintf(error, error_size, "%s", message);
}

static int read_csv_line(FILE *file, char *line)
{
    return read_line_bounded(file, line, CONFIG_CSV_MAX_LINE);
}

static int split_fields(char *line, char **fields, size_t count)
{
    size_t i;
    char *cursor = line;

    for (i = 0; i < count; i++) {
        char *comma;
        fields[i] = cursor;
        comma = strchr(cursor, ',');
        if (i + 1 < count) {
            if (comma == NULL) return 0;
            *comma = '\0';
            cursor = comma + 1;
        } else if (comma != NULL) {
            return 0;
        }
    }
    return 1;
}

static int parse_field(char *text, int min, int max, int *value,
                       size_t index, char *error, size_t error_size)
{
    InputStatus status;
    char message[160];

    if (*text == '\0') {
        snprintf(message, sizeof message, "CSV field '%s' is empty", field_names[index]);
        set_error(error, error_size, message);
        return 0;
    }
    status = parse_int_strict(text, min, max, value);
    if (status != INPUT_OK) {
        snprintf(message, sizeof message, "CSV field '%s' must be a whole number in [%d, %d]",
                 field_names[index], min, max);
        set_error(error, error_size, message);
        return 0;
    }
    return 1;
}

int config_csv_load(const char *path, SimConfig *config,
                    char *error, size_t error_size)
{
    FILE *file;
    char line[CONFIG_CSV_MAX_LINE];
    char *fields[10];
    SimConfig candidate;
    int status, d, value;
    char validation_error[128];

    if (error != NULL && error_size > 0) error[0] = '\0';
    if (path == NULL || config == NULL) {
        set_error(error, error_size, "configuration path or destination is missing");
        return 0;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        set_error(error, error_size, "could not open configuration file for reading");
        return 0;
    }

    status = read_csv_line(file, line);
    if (status != 1 || strcmp(line, csv_header) != 0) {
        set_error(error, error_size, status == 0 ? "CSV header line is oversized" :
                  "CSV header is missing, malformed, unknown, or duplicated");
        fclose(file);
        return 0;
    }
    status = read_csv_line(file, line);
    if (status != 1) {
        set_error(error, error_size, status == 0 ? "CSV configuration row is oversized" :
                  "CSV configuration row is missing or unreadable");
        fclose(file);
        return 0;
    }
    if (!split_fields(line, fields, sizeof fields / sizeof fields[0])) {
        set_error(error, error_size, "CSV row must contain exactly ten fields");
        fclose(file);
        return 0;
    }

    sim_config_default(&candidate);
    if (!parse_field(fields[0], SIM_MIN_DIRECTIONS, SIM_MAX_DIRECTIONS,
                     &candidate.num_directions, 0, error, error_size)) {
        fclose(file);
        return 0;
    }
    for (d = 0; d < SIM_MAX_DIRECTIONS; d++) candidate.arrival_rate[d] = 0;
    for (d = 0; d < SIM_MAX_DIRECTIONS; d++) {
        if (d < candidate.num_directions) {
            if (!parse_field(fields[d + 1], 0, SIM_MAX_ARRIVAL_RATE,
                             &candidate.arrival_rate[d], (size_t)d + 1,
                             error, error_size)) {
                fclose(file);
                return 0;
            }
        } else if (*fields[d + 1] != '\0') {
            char message[160];
            snprintf(message, sizeof message,
                     "CSV field '%s' must be empty for an inactive direction", field_names[d + 1]);
            set_error(error, error_size, message);
            fclose(file);
            return 0;
        }
    }
    if (!parse_field(fields[5], SIM_MIN_GREEN_TICKS, SIM_MAX_GREEN_TICKS,
                     &candidate.green_ticks, 5, error, error_size) ||
        !parse_field(fields[6], SIM_MIN_YELLOW_TICKS, SIM_MAX_YELLOW_TICKS,
                     &candidate.yellow_ticks, 6, error, error_size) ||
        !parse_field(fields[7], 1, QUEUE_CAPACITY,
                     &candidate.queue_capacity, 7, error, error_size) ||
        !parse_field(fields[8], 0, QUEUE_CAPACITY,
                     &candidate.initial_vehicles, 8, error, error_size) ||
        !parse_field(fields[9], SIM_MIN_TICKS, SIM_MAX_TICKS,
                     &candidate.total_ticks, 9, error, error_size)) {
        fclose(file);
        return 0;
    }
    value = read_csv_line(file, line);
    if (value != -1 || ferror(file)) {
        set_error(error, error_size, value == 0 ? "extra CSV line is oversized" :
                  "CSV file must contain exactly one configuration row");
        fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        set_error(error, error_size, "error closing configuration file after reading");
        return 0;
    }
    if (!sim_config_validate(&candidate, validation_error, sizeof validation_error)) {
        char message[192];
        snprintf(message, sizeof message, "invalid configuration: %s", validation_error);
        set_error(error, error_size, message);
        return 0;
    }
    *config = candidate;
    return 1;
}

int config_csv_save(const char *path, const SimConfig *config,
                    char *error, size_t error_size)
{
    FILE *file;
    int d;
    int ok;

    if (error != NULL && error_size > 0) error[0] = '\0';
    if (path == NULL || config == NULL ||
        !sim_config_validate(config, NULL, 0)) {
        set_error(error, error_size, "configuration is missing or invalid");
        return 0;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        set_error(error, error_size, "could not open configuration file for writing");
        return 0;
    }

    ok = fprintf(file, "%s\n%d", csv_header, config->num_directions) >= 0;
    for (d = 0; ok && d < SIM_MAX_DIRECTIONS; d++) {
        if (fputc(',', file) == EOF) ok = 0;
        if (ok && d < config->num_directions &&
            fprintf(file, "%d", config->arrival_rate[d]) < 0) ok = 0;
    }
    if (ok && fprintf(file, ",%d,%d,%d,%d,%d\n", config->green_ticks,
                      config->yellow_ticks, config->queue_capacity,
                      config->initial_vehicles, config->total_ticks) < 0) ok = 0;
    if (fflush(file) != 0 || ferror(file)) ok = 0;
    if (fclose(file) != 0) ok = 0;
    if (!ok) {
        set_error(error, error_size, "error writing configuration file");
        return 0;
    }
#ifndef _WIN32
    if (chmod(path, S_IRUSR | S_IWUSR) != 0) {
        set_error(error, error_size, "could not restrict configuration file permissions");
        return 0;
    }
#endif
    return 1;
}
