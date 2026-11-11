#include <stdio.h>
#include <string.h>

#include "input.h"
#include "queue.h"
#include "audit_log.h"
#include "multi_intersection.h"
#include "multi_intersection_cli.h"

static const char *direction_names[SIM_MAX_DIRECTIONS] = {
    "North", "South", "East", "West"
};

static int get_cli_input(const char *prompt, int min, int max, int *value)
{
    if (prompt_int(stdin, stdout, prompt, min, max, value) != 1) {
        fprintf(stderr, "\nError: Unable to read valid input.\n");
        return 0;
    }

    return 1;
}

static int choose_multi_run_length(int green_ticks, int yellow_ticks)
{
    int two_cycles =
        2 * SIM_MAX_DIRECTIONS * (green_ticks + yellow_ticks);

    return two_cycles > 60 ? two_cycles : 60;
}

static int configure_intersections(SimMultiIntersection *manager)
{
    SimConfig configs[SIM_MAX_INTERSECTIONS];
    int count;
    int i;
    int initial_vehicles;
    int green_ticks;
    int density;

    printf("\n========== MULTI-INTERSECTION CONFIGURATION ==========\n");

    if (!get_cli_input(
            "Number of intersections (1-3): ",
            SIM_MIN_INTERSECTIONS,
            SIM_MAX_INTERSECTIONS,
            &count)) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        printf("\n---------- INTERSECTION %d ----------\n", i + 1);

        if (!get_cli_input(
                "Initial vehicles per direction (1-100): ",
                1, QUEUE_CAPACITY, &initial_vehicles)) {
            return 0;
        }

        if (!get_cli_input(
                "Green signal duration (1-120 seconds): ",
                1, SIM_MAX_GREEN_TICKS, &green_ticks)) {
            return 0;
        }

        if (!get_cli_input(
                "Traffic density (0-100 vehicles per direction per 100 seconds): ",
                0, SIM_MAX_ARRIVAL_RATE, &density)) {
            return 0;
        }

        sim_config_default(&configs[i]);

        configs[i].num_directions = SIM_MAX_DIRECTIONS;
        configs[i].initial_vehicles = initial_vehicles;
        configs[i].green_ticks = green_ticks;
        configs[i].total_ticks = choose_multi_run_length(
            green_ticks, configs[i].yellow_ticks);

        sim_config_set_traffic_density(&configs[i], density);

        printf("Intersection %d configuration saved.\n", i + 1);
    }

    if (!sim_multi_init(manager, count, configs)) {
        fprintf(stderr, "Error: Could not initialize intersections.\n");
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (!audit_log_config_change(
                "traffic_audit.log",
                manager->intersections[i].id,
                "Multi-intersection configuration created")) {
            fprintf(
                stderr,
                "Warning: Could not audit configuration for "
                "intersection %d.\n",
                manager->intersections[i].id);
        }
    }

    return 1;
}

static const char *session_state_name(SimSessionState state)
{
    switch (state) {
        case SIM_SESSION_IDLE:      return "Idle";
        case SIM_SESSION_RUNNING:   return "Running";
        case SIM_SESSION_PAUSED:    return "Paused";
        case SIM_SESSION_STOPPED:   return "Stopped";
        case SIM_SESSION_COMPLETED: return "Completed";
        default:                    return "Unknown";
    }
}

static void display_multi_live_status(
    const SimMultiIntersection *manager)
{
    int i;
    int d;

    printf("\n================ MULTI-INTERSECTION LIVE STATUS ================\n");

    for (i = 0; i < manager->count; i++) {
        const SimIntersection *intersection = &manager->intersections[i];
        const SimSession *session = &intersection->session;
        unsigned int tick = session->next_tick > 0
            ? session->next_tick - 1
            : 0;
        unsigned long dropped = 0;
        unsigned long waiting = 0;

        printf("\nINTERSECTION %d | %s | Tick %u/%d\n",
               intersection->id,
               session_state_name(session->state),
               session->next_tick,
               session->config.total_ticks);

        printf("%-8s %-10s %-10s %-10s\n",
               "Direction", "Signal", "Remaining", "Queue");

        for (d = 0; d < session->config.num_directions; d++) {
            const VehicleQueue *queue = &session->queues[d];

            printf("%-8s %-10s %-10d %-10d\n",
                   direction_names[d],
                   sim_signal_name(
                       sim_signal_at(&session->config, d, tick)),
                   sim_signal_remaining_at(&session->config, d, tick),
                   queue->size);

            dropped += (unsigned long)queue->dropped;
            waiting += (unsigned long)queue->size;
        }

        printf("Generated: %lu | Passed: %lu | Dropped: %lu | Waiting: %lu\n",
               intersection->stats.generated,
               intersection->stats.passed,
               dropped,
               waiting);
    }

    printf("\n==================================================================\n");
}

static void display_multi_signal_audit(
    const SimIntersection *intersection)
{
    size_t i;
    const SimSignalAuditEntry *entry;
    const SimSession *session = &intersection->session;

    printf("\n========== INTERSECTION %d SIGNAL AUDIT ==========\n",
           intersection->id);

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
                       direction_names[entry->direction],
                       sim_signal_name(entry->previous_signal),
                       sim_signal_name(entry->new_signal));
            }
        }
    }

    printf("===================================================\n");
}

static int has_active_intersections(
    const SimMultiIntersection *manager)
{
    int i;

    for (i = 0; i < manager->count; i++) {
        SimSessionState state =
            manager->intersections[i].session.state;

        if (state == SIM_SESSION_RUNNING ||
            state == SIM_SESSION_PAUSED) {
            return 1;
        }
    }

    return 0;
}

static void stop_active_intersections(
    SimMultiIntersection *manager)
{
    int i;

    for (i = 0; i < manager->count; i++) {
        SimSessionState state =
            manager->intersections[i].session.state;

        if (state == SIM_SESSION_RUNNING ||
            state == SIM_SESSION_PAUSED) {
            (void)sim_multi_stop(manager,
                                 manager->intersections[i].id);
        }
    }
}

static int parse_intersection_command(
    const char *command,
    const char *verb,
    int count,
    int *intersection_id)
{
    size_t length = strlen(verb);

    if (strncmp(command, verb, length) != 0 ||
        command[length] != ' ') {
        return 0;
    }

    return parse_int_strict(
        command + length + 1,
        1, count, intersection_id) == INPUT_OK;
}

static int read_multi_command(char *command, size_t size)
{
    size_t length;

    if (fgets(command, (int)size, stdin) == NULL) {
        return 0;
    }

    length = strlen(command);

    while (length > 0 &&
           (command[length - 1] == '\n' ||
            command[length - 1] == '\r')) {
        command[--length] = '\0';
    }

    if (length == size - 1 &&
        command[length - 1] != '\n') {
        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard the rest of an oversized command. */
        }

        command[0] = '\0';
        return -1;
    }

    return 1;
}

static void display_multi_final_reports(
    SimMultiIntersection *manager)
{
    int i;
    const SimStats *combined;

    printf("\n\n================ INDIVIDUAL INTERSECTION REPORTS ================\n");

    for (i = 0; i < manager->count; i++) {
        printf("\nINTERSECTION %d FINAL REPORT\n",
               manager->intersections[i].id);

        report_print(stdout, &manager->intersections[i].stats);
        display_multi_signal_audit(&manager->intersections[i]);
    }

    printf("\n================ COMBINED TRAFFIC REPORT ================\n");

    combined = sim_multi_get_combined_stats(manager);

    if (combined != NULL) {
        report_print(stdout, combined);

        printf("\nVehicle conservation: %s\n",
               stats_is_conserved(combined) ? "PASSED" : "FAILED");
    }

    printf("==========================================================\n");
}

static void persist_multi_intersection_audit(
    SimMultiIntersection *manager,
    size_t logged_counts[SIM_MAX_INTERSECTIONS],
    int warning_shown[SIM_MAX_INTERSECTIONS])
{
    int i;

    for (i = 0; i < manager->count; i++) {
        SimIntersection *intersection =
            &manager->intersections[i];

        size_t audit_count =
            sim_session_audit_count(&intersection->session);

        while (logged_counts[i] < audit_count) {
            const SimSignalAuditEntry *entry =
                sim_session_audit_get(
                    &intersection->session,
                    logged_counts[i]);

            if (entry == NULL ||
                !audit_log_signal(
                    "traffic_audit.log",
                    intersection->id,
                    entry->tick,
                    entry->direction,
                    entry->previous_signal,
                    entry->new_signal)) {

                if (!warning_shown[i]) {
                    fprintf(
                        stderr,
                        "\nWarning: Could not persist audit events "
                        "for intersection %d.\n",
                        intersection->id);

                    warning_shown[i] = 1;
                }

                break;
            }

            logged_counts[i]++;
        }
    }
}

static int run_multi_controls(SimMultiIntersection *manager)
{
    char command[64];
    int automatic = 0;
    int started = 0;
    int result;
    int intersection_id;
    int i;
    size_t logged_counts[SIM_MAX_INTERSECTIONS] = {0};
    int audit_warning_shown[SIM_MAX_INTERSECTIONS] = {0};

    printf("\nType start to begin (or quit to exit): ");

    result = read_multi_command(command, sizeof command);

    if (result == 0 ||
    (result == 1 && strcmp(command, "") == 0)) {
    automatic = 1;
    } else if (result < 0) {
        printf("Error: command is too long.\n");
        return 0;
    } else if (strcmp(command, "quit") == 0) {
        return 1;
    } else if (strcmp(command, "start") != 0) {
        printf("Error: enter start or quit.\n");
        return 0;
    }

    if (!sim_multi_start(manager)) {
        fprintf(stderr, "Error: Could not start all intersections.\n");
        return 0;
    }

    started = 1;
    printf("\nAll %d intersections started.\n", manager->count);

        persist_multi_intersection_audit(
        manager,
        logged_counts,
        audit_warning_shown);

        if (automatic) {
            for (;;) {
                result = sim_multi_step(manager);

                persist_multi_intersection_audit(
                manager,
                logged_counts,
                audit_warning_shown);

                if (!result) {
                break;
            }

                display_multi_live_status(manager);
        }
        } else {
        while (has_active_intersections(manager)) {
            printf("\nCommands: step | pause N | resume N | stop N | quit\n");
            printf("multi> ");

            result = read_multi_command(command, sizeof command);

            if (result == 0) {
                printf("\nInput ended; stopping active intersections safely.\n");
                stop_active_intersections(manager);
                break;
            }

            if (result < 0) {
                printf("Error: command is too long.\n");
                continue;
            }

            if (strcmp(command, "") == 0 ||
                strcmp(command, "step") == 0) {

                result = sim_multi_step(manager);

                persist_multi_intersection_audit(
                    manager,
                    logged_counts,
                    audit_warning_shown);

                if (result) {
                    display_multi_live_status(manager);
                } else {
                    printf("No running intersection could advance. "
                           "Resume a paused intersection or stop it.\n");
                }
            } else if (strcmp(command, "quit") == 0) {
                stop_active_intersections(manager);
                printf("All active intersections stopped.\n");
                break;
            } else if (parse_intersection_command(
                           command, "pause", manager->count,
                           &intersection_id)) {
                if (sim_multi_pause(manager, intersection_id)) {
                    printf("Intersection %d paused.\n", intersection_id);
                } else {
                    printf("Could not pause intersection %d.\n",
                           intersection_id);
                }
            } else if (parse_intersection_command(
                           command, "resume", manager->count,
                           &intersection_id)) {
                if (sim_multi_resume(manager, intersection_id)) {
                    printf("Intersection %d resumed.\n", intersection_id);
                } else {
                    printf("Could not resume intersection %d.\n",
                           intersection_id);
                }
            } else if (parse_intersection_command(
                           command, "stop", manager->count,
                           &intersection_id)) {
                if (sim_multi_stop(manager, intersection_id)) {
                    printf("Intersection %d stopped.\n", intersection_id);
                } else {
                    printf("Could not stop intersection %d.\n",
                           intersection_id);
                }
            } else {
                printf("Error: invalid command or intersection ID.\n");
            }
        }
    }

    if (!started) {
        return 1;
    }

    display_multi_final_reports(manager);

    if (!sim_multi_is_conserved(manager)) {
        fprintf(stderr,
                "Error: Vehicle conservation failed for at least one intersection.\n");
        return 0;
    }

    for (i = 0; i < manager->count; i++) {
        printf("Intersection %d vehicle conservation: %s\n",
               manager->intersections[i].id,
               stats_is_conserved(
                   &manager->intersections[i].stats)
                   ? "PASSED" : "FAILED");
    }

    return 1;
}

int runMultiIntersectionMode(void)
{
    SimMultiIntersection manager = {0};

    if (!configure_intersections(&manager)) {
        return 0;
    }

    printf("\nMulti-intersection configuration completed successfully.\n");

    return run_multi_controls(&manager);
}
