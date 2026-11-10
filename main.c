
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "input.h"
#include "config_file.h"
#include "queue.h"
#include "simulation.h"
#include "stats.h"
#include "multi_intersection_cli.h"

#define NUM_DIRECTIONS SIM_MAX_DIRECTIONS

enum Direction {
    NORTH = 0,
    SOUTH,
    EAST,
    WEST
};

static const char *directionNames[NUM_DIRECTIONS] = {
    "North", "South", "East", "West"
};

static int getValidatedInput(const char *prompt, int min, int max)
{
    int value;

    /* Bounded, strict parsing lives in input.c so it can be unit tested. */
    if (prompt_int(stdin, stdout, prompt, min, max, &value) != 1) {
        fprintf(stderr, "Error: Unable to read input.\n");
        exit(EXIT_FAILURE);
    }

    return value;
}

/* Called after every simulation tick to display live traffic status. */
static void displayTick(const SimTickInfo *info, void *user)
{
    int i;
    int arrived = 0;
    int passed = 0;
    int dropped = 0;
    int totalWaiting = 0;
    const SimStats *stats = (const SimStats *)user;

    printf("\n========== LIVE TRAFFIC STATUS ==========\n");
    printf("Tick: %03u\n", info->tick);

    for (i = 0; i < info->num_directions; i++) {
        printf("%-5s | Signal: %-6s | Remaining: %2ds | Queue: %d\n",
               directionNames[i],
               sim_signal_name(info->signal[i]),
               info->remaining_duration[i],
               info->queue_length[i]);

        arrived += info->generated[i];
        passed += info->departed[i];
        dropped += info->dropped[i];
        totalWaiting += info->queue_length[i];
    }

    printf("-----------------------------------------\n");
    printf("Vehicles arrived this tick : %d\n", arrived);
    printf("Vehicles passed this tick  : %d\n", passed);
    printf("Vehicles dropped this tick : %d\n", dropped);

    if (stats != NULL) {
        printf("Total vehicles passed      : %lu\n", stats->passed);
    }

    printf("Total vehicles waiting     : %d\n", totalWaiting);
    printf("=========================================\n");
}


/* Display the chronological traffic signal audit history. */
static void displaySignalAudit(const SimSession *session)
{
    size_t i;
    const SimSignalAuditEntry *entry;

    printf("\n========== SIGNAL CHANGE AUDIT TRAIL ==========\n");

    if (sim_session_audit_count(session) == 0) {
        printf("No signal changes recorded.\n");
    } else {
        printf("%-8s %-12s %-12s %-12s\n",
               "Tick", "Direction", "Previous", "New");

        for (i = 0; i < sim_session_audit_count(session); i++) {
            entry = sim_session_audit_get(session, i);

            if (entry != NULL) {
                printf("%-8u %-12s %-12s %-12s\n",
                       entry->tick,
                       directionNames[entry->direction],
                       sim_signal_name(entry->previous_signal),
                       sim_signal_name(entry->new_signal));
            }
        }
    }

    printf("================================================\n");
}



static void displayConfiguration(const SimConfig *config)
{
    int d;
    printf("\n========== SETTINGS FOR NEXT RUN ==========\n");
    printf("Directions: %d\n", config->num_directions);
    for (d = 0; d < config->num_directions; d++)
        printf("%-5s | Density: %d vehicles per 100 seconds\n",
               directionNames[d], config->arrival_rate[d]);
    printf("Green: %d seconds | Yellow: %d seconds\n",
           config->green_ticks, config->yellow_ticks);
    printf("Queue capacity: %d | Initial vehicles: %d per direction\n",
           config->queue_capacity, config->initial_vehicles);
    printf("Run length: %d seconds\n", config->total_ticks);
    printf("===========================================\n");
}

static int configureOption(SimConfig *config, const char *command)
{
    int value, direction;
    char prompt[128];
    if (strcmp(command, "density") == 0) {
        direction = getValidatedInput("Direction (1-North, 2-South, 3-East, 4-West): ", 1, config->num_directions) - 1;
        snprintf(prompt, sizeof prompt, "%s density (0-%d vehicles per 100 seconds): ", directionNames[direction], SIM_MAX_ARRIVAL_RATE);
        value = getValidatedInput(prompt, 0, SIM_MAX_ARRIVAL_RATE);
        config->arrival_rate[direction] = value;
    } else if (strcmp(command, "directions") == 0) {
        value = getValidatedInput("Number of directions (2-4): ", SIM_MIN_DIRECTIONS, SIM_MAX_DIRECTIONS);
        config->num_directions = value;
    } else if (strcmp(command, "green") == 0) {
        value = getValidatedInput("Green duration (1-120 seconds): ", SIM_MIN_GREEN_TICKS, SIM_MAX_GREEN_TICKS);
        config->green_ticks = value;
    } else if (strcmp(command, "yellow") == 0) {
        value = getValidatedInput("Yellow duration (1-30 seconds): ", SIM_MIN_YELLOW_TICKS, SIM_MAX_YELLOW_TICKS);
        config->yellow_ticks = value;
    } else if (strcmp(command, "capacity") == 0) {
        value = getValidatedInput("Queue capacity (at least current initial vehicles, at most 100): ", config->initial_vehicles > 1 ? config->initial_vehicles : 1, QUEUE_CAPACITY);
        config->queue_capacity = value;
    } else if (strcmp(command, "initial") == 0) {
        value = getValidatedInput("Initial vehicles per direction: ", 0, config->queue_capacity);
        config->initial_vehicles = value;
    } else if (strcmp(command, "length") == 0) {
        value = getValidatedInput("Run length (1-10000 seconds): ", SIM_MIN_TICKS, SIM_MAX_TICKS);
        config->total_ticks = value;
    } else {
        return 0;
    }
    return 1;
}

static int readPath(const char *prompt, char *path, size_t size)
{
    int status;
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        status = read_line_bounded(stdin, path, size);
        if (status < 0) return 0;
        if (status == 0) {
            printf("Error: path is too long (maximum %lu characters).\n",
                   (unsigned long)(size - 1));
        } else if (path[0] == '\0') {
            printf("Error: path cannot be empty.\n");
        } else {
            return 1;
        }
    }
}

static int confirm(const char *prompt)
{
    char answer[16];
    int status;
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        status = read_command_line(stdin, answer, sizeof answer);
        if (status == 0) return 0;
        if (status < 0) printf("Error: answer is too long.\n");
        else if (strcmp(answer, "yes") == 0 || strcmp(answer, "y") == 0) return 1;
        else if (strcmp(answer, "no") == 0 || strcmp(answer, "n") == 0) return 0;
        else printf("Enter yes or no.\n");
    }
}

static int loadConfiguration(SimConfig *config)
{
    char path[INPUT_MAX_LINE], error[192];
    SimConfig candidate;
    if (!readPath("Configuration CSV path: ", path, sizeof path)) return 0;
    candidate = *config;
    if (!config_csv_load(path, &candidate, error, sizeof error)) {
        printf("Load failed: %s. Current settings were kept.\n", error);
        return 0;
    }
    *config = candidate;
    printf("Configuration loaded.\n");
    displayConfiguration(config);
    return 1;
}

static void saveConfiguration(const SimConfig *config)
{
    char path[INPUT_MAX_LINE], error[192];
    struct stat info;
    for (;;) {
        if (!readPath("Configuration CSV path: ", path, sizeof path)) return;
        if (stat(path, &info) == 0 &&
            !confirm("File exists. Overwrite it? (yes/no): ")) {
            printf("Save cancelled; existing file was kept.\n");
            return;
        }
        if (config_csv_save(path, config, error, sizeof error)) {
            printf("Configuration saved to '%s'.\n", path);
            return;
        }
        printf("Save failed: %s.\n", error);
        if (!confirm("Retry with another path? (yes/no): ")) return;
    }
}

static void editConfiguration(SimConfig *config)
{
    char command[32];
    int status;
    for (;;) {
        printf("\nEdit setting: density, directions, green, yellow, capacity, initial, length, done: ");
        status = read_command_line(stdin, command, sizeof command);
        if (status == 0 || (status == 1 && strcmp(command, "done") == 0)) return;
        if (status < 0) printf("Error: command is too long.\n");
        else if (!configureOption(config, command))
            printf("Error: choose a listed setting or done.\n");
    }
}

static int runOne(const SimConfig *config, SimStats *stats)
{
    SimSession session = {0};
    char command[32];
    int status;

    displayConfiguration(config);
    if (!sim_session_start(&session, config, stats, displayTick, stats)) {
        fprintf(stderr, "Error: Simulation could not start.\n");
        return 0;
    }
    printf("Started. Enter or step advances one tick; pause, resume, and stop control the run.\n");
    while (session.state == SIM_SESSION_RUNNING || session.state == SIM_SESSION_PAUSED) {
        printf("[%s, tick %u/%d] command> ",
               session.state == SIM_SESSION_RUNNING ? "running" : "paused",
               session.next_tick, session.config.total_ticks);
        status = read_command_line(stdin, command, sizeof command);
        if (status < 0) {
            printf("Error: command is too long.\n");
            continue;
        }
        if (status == 0) {
            (void)sim_session_stop(&session);
            printf("\nInput ended; stopped safely at tick %u.\n", stats->ticks);
            break;
        }
        if (strcmp(command, "") == 0 || strcmp(command, "step") == 0) {
            if (!sim_session_step(&session)) printf("Error: session cannot advance now.\n");
        } else if (strcmp(command, "pause") == 0) {
            if (!sim_session_pause(&session)) printf("Error: pause is only valid while running.\n");
            else printf("Paused at tick %u. Generation and signal processing are stopped.\n", session.next_tick);
        } else if (strcmp(command, "resume") == 0) {
            if (!sim_session_resume(&session)) printf("Error: resume is only valid while paused.\n");
            else printf("Resumed at tick %u.\n", session.next_tick);
        } else if (strcmp(command, "stop") == 0) {
            if (sim_session_stop(&session)) printf("Stopped safely at tick %u.\n", stats->ticks);
            else printf("Error: stop is only valid for an active session.\n");
        } else {
            printf("Error: use step, pause, resume, or stop.\n");
        }
    }
    printf("\n");
    displaySignalAudit(&session);
    report_print(stdout, stats);
    return stats_is_conserved(stats);
}

static int runWithControls(SimConfig *config, SimStats *stats)
{
    char command[32];
    int has_run = 0, status;
    for (;;) {
        if (!has_run) printf("\nConfiguration: load, edit, start, quit: ");
        else printf("\nNext: start, edit, load, save, quit: ");
        status = read_command_line(stdin, command, sizeof command);
        if (status == 0) return 1;
        if (status < 0) printf("Error: command is too long.\n");
        else if (strcmp(command, "quit") == 0) return 1;
        else if (strcmp(command, "load") == 0) (void)loadConfiguration(config);
        else if (strcmp(command, "edit") == 0) {
            editConfiguration(config);
            displayConfiguration(config);
        } else if (strcmp(command, "start") == 0) {
            if (!runOne(config, stats)) return 0;
            has_run = 1;
            if (confirm("Save effective configuration? (yes/no): "))
                saveConfiguration(config);
        } else if (has_run && strcmp(command, "save") == 0) {
            saveConfiguration(config);
        } else printf("Error: enter load, edit, start, or quit.\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--multi") == 0) {
        return runMultiIntersectionMode()
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }

    if (argc != 1) {
        fprintf(stderr, "Usage: traffic_sim.exe [--multi]\n");
        return EXIT_FAILURE;
    }
    SimConfig config;
    SimStats stats;
    char error[96];

    printf("=== TRAFFIC MANAGEMENT CONFIGURATION ===\n");
    sim_config_default(&config);
    if (!sim_config_validate(&config, error, sizeof error)) {
        fprintf(stderr, "Error: %s.\n", error);
        return EXIT_FAILURE;
    }

    printf("\nTRAFFIC MANAGEMENT SIMULATION\n");
    printf("Use load, edit, start, and quit to configure and run the simulation.\n");
    return runWithControls(&config, &stats) ? EXIT_SUCCESS : EXIT_FAILURE;
}
