
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "queue.h"
#include "simulation.h"
#include "stats.h"

#define NUM_DIRECTIONS SIM_MAX_DIRECTIONS
#define MAX_SIGNAL_DURATION SIM_MAX_GREEN_TICKS
#define MIN_RUN_TICKS 60   /* one tick is one simulated second */

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



/* Long enough for every direction to get at least two green phases. */
static int chooseRunLength(int greenTicks, int yellowTicks)
{
    int twoCycles = 2 * NUM_DIRECTIONS * (greenTicks + yellowTicks);

    return twoCycles > MIN_RUN_TICKS ? twoCycles : MIN_RUN_TICKS;
}

/* Line-based commands keep control portable across the supported CI systems. */
static int readControlCommand(char *command, size_t size)
{
    size_t length;

    if (fgets(command, (int)size, stdin) == NULL) return 0;

    length = strlen(command);

    while (length > 0 &&
           (command[length - 1] == '\n' || command[length - 1] == '\r')) {
        command[--length] = '\0';
    }

    return 1;
}


static int runWithControls(const SimConfig *config, SimStats *stats)
{
    SimSession session = {0};
    char command[32];
    int automatic = 0;
    int has_run = 0;

    for (;;) {
        printf("\nType start to begin (or quit to exit): ");
        if (!readControlCommand(command, sizeof command)) {
            if (has_run) return 1;
            if (automatic) return 1;
            /* Preserve the historical config-only piped workflow used by CI. */
            automatic = 1;
            memcpy(command, "start", sizeof "start");
        }
        if (strcmp(command, "quit") == 0) return 1;
        if (strcmp(command, "start") != 0) {
            printf("Error: enter start or quit.\n");
            continue;
        }
        if (!sim_session_start(&session, config, stats, displayTick, stats)) {
            fprintf(stderr, "Error: Simulation could not start.\n");
            return 0;
        }
        has_run = 1;
        printf("Started. Enter or step advances one tick; pause, resume, and stop control the run.\n");

        while (session.state == SIM_SESSION_RUNNING ||
               session.state == SIM_SESSION_PAUSED) {
            if (session.state == SIM_SESSION_RUNNING && automatic) {
                (void)sim_session_step(&session);
                continue;
            }
            printf("[%s, tick %u/%d] command> ",
                   session.state == SIM_SESSION_RUNNING ? "running" : "paused",
                   session.next_tick, config->total_ticks);
            if (!readControlCommand(command, sizeof command)) {
                if (session.state == SIM_SESSION_RUNNING ||
                    session.state == SIM_SESSION_PAUSED) {
                    (void)sim_session_stop(&session);
                    printf("\nInput ended; stopped safely at tick %u.\n", stats->ticks);
                }
                break;
            }
            if (strcmp(command, "") == 0 || strcmp(command, "step") == 0) {
                if (!sim_session_step(&session)) {
                    printf("Error: the session cannot advance in its current state.\n");
                }
            } else if (strcmp(command, "pause") == 0) {
                if (!sim_session_pause(&session)) {
                    printf("Error: pause is only valid while running.\n");
                } else {
                    printf("Paused at tick %u. Generation and signal processing are stopped.\n",
                           session.next_tick);
                }
            } else if (strcmp(command, "resume") == 0) {
                if (!sim_session_resume(&session)) {
                    printf("Error: resume is only valid while paused.\n");
                } else {
                    printf("Resumed at tick %u.\n", session.next_tick);
                }
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
        if (!stats_is_conserved(stats)) return 0;
        if (automatic) return 1;
        if (session.state == SIM_SESSION_COMPLETED) printf("Run complete. ");
        else printf("Run stopped. ");
        printf("Type start to run again, or quit to exit.\n");
    }
}

int main(void)
{
    SimConfig config;
    SimStats stats;
    char error[96];
    int initialVehicles;
    int signalDuration;
    int trafficDensity;

    printf("=== TRAFFIC MANAGEMENT CONFIGURATION ===\n");

    initialVehicles = getValidatedInput(
        "Enter initial vehicles per direction (1-100): ",
        1, QUEUE_CAPACITY);

    signalDuration = getValidatedInput(
        "Enter green signal duration (1-120 seconds): ",
        1, MAX_SIGNAL_DURATION);

    trafficDensity = getValidatedInput(
        "Enter traffic density (0-100 vehicles per direction per 100 seconds): ",
        0, SIM_MAX_ARRIVAL_RATE);

    sim_config_default(&config);
    config.num_directions = NUM_DIRECTIONS;
    config.initial_vehicles = initialVehicles;
    config.green_ticks = signalDuration;
    config.total_ticks = chooseRunLength(signalDuration, config.yellow_ticks);

    sim_config_set_traffic_density(&config, trafficDensity);

    if (!sim_config_validate(&config, error, sizeof error)) {
        fprintf(stderr, "Error: %s.\n", error);
        return EXIT_FAILURE;
    }

    printf("\nConfiguration saved for this run.\n");
    printf("Initial vehicles: %d per direction\n", initialVehicles);
    printf("Green signal duration: %d seconds per direction\n",
           signalDuration);
    printf("Traffic density: %d vehicles per direction per 100 seconds\n",
           trafficDensity);
    printf("Simulated time: %d seconds\n", config.total_ticks);

    printf("\nTRAFFIC MANAGEMENT SIMULATION\n");
    printf("Controls use whole-line commands between ticks; no background timer is used.\n");
    return runWithControls(&config, &stats) ? EXIT_SUCCESS : EXIT_FAILURE;
}
