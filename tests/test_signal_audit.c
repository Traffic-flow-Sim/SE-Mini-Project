
/*
 * Tests for S1TMS-15: Traffic signal change logging and audit trail.
 */
#include <stdio.h>

#include "simulation.h"
#include "test_util.h"

static SimConfig audit_test_config(void)
{
    SimConfig config;

    sim_config_default(&config);

    config.num_directions = 2;
    config.initial_vehicles = 0;
    config.queue_capacity = QUEUE_CAPACITY;
    sim_config_set_traffic_density(&config, 0);
    config.green_ticks = 2;
    config.yellow_ticks = 1;
    config.discharge_per_tick = 1;
    config.total_ticks = 6;

    return config;
}

static void test_exact_signal_transitions_are_recorded(void)
{
    SimConfig config = audit_test_config();
    SimStats stats;
    SimSession session = {0};
    const SimSignalAuditEntry *entry;
    int i;

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);

    for (i = 0; i < config.total_ticks; i++) {
        CHECK(sim_session_step(&session) == 1);
    }

    CHECK(session.state == SIM_SESSION_COMPLETED);
    CHECK(sim_session_audit_count(&session) == 5);

    /* Tick 0: Direction 0 changes from RED to GREEN. */
    entry = sim_session_audit_get(&session, 0);
    CHECK(entry != NULL);
    if (entry != NULL) {
        CHECK(entry->tick == 0);
        CHECK(entry->direction == 0);
        CHECK(entry->previous_signal == SIM_SIGNAL_RED);
        CHECK(entry->new_signal == SIM_SIGNAL_GREEN);
    }

    /* Tick 2: Direction 0 changes from GREEN to YELLOW. */
    entry = sim_session_audit_get(&session, 1);
    CHECK(entry != NULL);
    if (entry != NULL) {
        CHECK(entry->tick == 2);
        CHECK(entry->direction == 0);
        CHECK(entry->previous_signal == SIM_SIGNAL_GREEN);
        CHECK(entry->new_signal == SIM_SIGNAL_YELLOW);
    }

    /* Tick 3: Direction 0 changes to RED. */
    entry = sim_session_audit_get(&session, 2);
    CHECK(entry != NULL);
    if (entry != NULL) {
        CHECK(entry->tick == 3);
        CHECK(entry->direction == 0);
        CHECK(entry->previous_signal == SIM_SIGNAL_YELLOW);
        CHECK(entry->new_signal == SIM_SIGNAL_RED);
    }

    /* Tick 3: Direction 1 changes from RED to GREEN. */
    entry = sim_session_audit_get(&session, 3);
    CHECK(entry != NULL);
    if (entry != NULL) {
        CHECK(entry->tick == 3);
        CHECK(entry->direction == 1);
        CHECK(entry->previous_signal == SIM_SIGNAL_RED);
        CHECK(entry->new_signal == SIM_SIGNAL_GREEN);
    }

    /* Tick 5: Direction 1 changes from GREEN to YELLOW. */
    entry = sim_session_audit_get(&session, 4);
    CHECK(entry != NULL);
    if (entry != NULL) {
        CHECK(entry->tick == 5);
        CHECK(entry->direction == 1);
        CHECK(entry->previous_signal == SIM_SIGNAL_GREEN);
        CHECK(entry->new_signal == SIM_SIGNAL_YELLOW);
    }
}

static void test_audit_entries_are_chronological(void)
{
    SimConfig config = audit_test_config();
    SimStats stats;
    SimSession session = {0};
    const SimSignalAuditEntry *previous;
    const SimSignalAuditEntry *current;
    size_t i;

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);

    while (session.state == SIM_SESSION_RUNNING) {
        CHECK(sim_session_step(&session) == 1);
    }

    for (i = 1; i < sim_session_audit_count(&session); i++) {
        previous = sim_session_audit_get(&session, i - 1);
        current = sim_session_audit_get(&session, i);

        CHECK(previous != NULL);
        CHECK(current != NULL);

        if (previous != NULL && current != NULL) {
            CHECK(previous->tick <= current->tick);
        }
    }
}

static void test_invalid_audit_requests(void)
{
    SimConfig config = audit_test_config();
    SimStats stats;
    SimSession session = {0};

    CHECK(sim_session_audit_count(NULL) == 0);
    CHECK(sim_session_audit_get(NULL, 0) == NULL);

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    CHECK(sim_session_audit_count(&session) == 0);
    CHECK(sim_session_audit_get(&session, 0) == NULL);

    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_audit_count(&session) == 1);
    CHECK(sim_session_audit_get(&session, 1) == NULL);
}

static void test_audit_history_resets_when_session_restarts(void)
{
    SimConfig config = audit_test_config();
    SimStats stats;
    SimSession session = {0};

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_audit_count(&session) == 1);

    CHECK(sim_session_stop(&session) == 1);
    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);

    CHECK(sim_session_audit_count(&session) == 0);
    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_audit_count(&session) == 1);
}

int main(void)
{
    TEST_RUN(test_exact_signal_transitions_are_recorded);
    TEST_RUN(test_audit_entries_are_chronological);
    TEST_RUN(test_invalid_audit_requests);
    TEST_RUN(test_audit_history_resets_when_session_restarts);

    return TEST_SUMMARY();
}
