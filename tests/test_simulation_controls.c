#include "simulation.h"
#include "test_util.h"

#include <string.h>

typedef struct {
    unsigned int ticks[8];
    int count;
} TickLog;

static void record_tick(const SimTickInfo *info, void *user)
{
    TickLog *log = (TickLog *)user;
    if (log->count < 8) log->ticks[log->count++] = info->tick;
}

static SimConfig short_config(void)
{
    SimConfig config;
    sim_config_default(&config);
    config.num_directions = 2;
    config.initial_vehicles = 2;
    sim_config_set_traffic_density(&config, 0);
    config.total_ticks = 4;
    return config;
}

static void test_start_pause_resume_preserves_progress(void)
{
    SimConfig config = short_config();
    SimStats stats;
    SimSession session = {0};
    TickLog log = {{0}, 0};

    CHECK(sim_session_pause(&session) == 0);
    CHECK(sim_session_resume(&session) == 0);
    CHECK(sim_session_stop(&session) == 0);
    CHECK(sim_session_start(&session, &config, &stats, record_tick, &log) == 1);
    CHECK(session.state == SIM_SESSION_RUNNING);
    CHECK(stats.ticks == 0);
    CHECK(sim_session_step(&session) == 1);
    CHECK(log.count == 1 && log.ticks[0] == 0);
    {
        SimStats before_stats = stats;
        VehicleQueue before_queues[SIM_MAX_DIRECTIONS];
        VehicleGenerator before_generator = session.generator;
        unsigned int before_next_tick = session.next_tick;

        memcpy(before_queues, session.queues, sizeof before_queues);
        CHECK(sim_session_pause(&session) == 1);
        CHECK(session.state == SIM_SESSION_PAUSED);
        CHECK(sim_session_step(&session) == 0);
        CHECK(memcmp(&stats, &before_stats, sizeof stats) == 0);
        CHECK(memcmp(session.queues, before_queues, sizeof before_queues) == 0);
        CHECK(memcmp(&session.generator, &before_generator, sizeof before_generator) == 0);
        CHECK(session.next_tick == before_next_tick);
    }
    CHECK(log.count == 1);
    CHECK(sim_session_pause(&session) == 0);
    CHECK(sim_session_start(&session, &config, &stats, record_tick, &log) == 0);
    CHECK(log.count == 1);
    CHECK(sim_session_resume(&session) == 1);
    CHECK(session.state == SIM_SESSION_RUNNING);
    CHECK(sim_session_step(&session) == 1);
    CHECK(log.count == 2 && log.ticks[1] == 1);
}

static void test_pause_resume_matches_uninterrupted_vehicle_flow(void)
{
    SimConfig config = short_config();
    SimStats baseline;
    SimStats resumed;
    SimSession session = {0};
    int i;

    CHECK(sim_run(&config, &baseline, NULL, NULL) == 1);
    CHECK(sim_session_start(&session, &config, &resumed, NULL, NULL) == 1);
    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_pause(&session) == 1);
    CHECK(sim_session_step(&session) == 0);
    CHECK(sim_session_resume(&session) == 1);
    for (i = 1; i < config.total_ticks; i++) {
        if (i == 2) {
            CHECK(sim_session_pause(&session) == 1);
            CHECK(sim_session_resume(&session) == 1);
        }
        CHECK(sim_session_step(&session) == 1);
    }
    CHECK(session.state == SIM_SESSION_COMPLETED);
    CHECK(resumed.generated == baseline.generated);
    CHECK(resumed.passed == baseline.passed);
    CHECK(resumed.dropped == baseline.dropped);
    CHECK(resumed.remaining == baseline.remaining);
    CHECK(resumed.total_wait == baseline.total_wait);
    CHECK(resumed.max_queue_length == baseline.max_queue_length);
    CHECK(resumed.ticks == baseline.ticks);
    CHECK(session.generator.next_id == (unsigned int)baseline.generated + 1);
}

static void test_stop_while_paused_finalizes_partial_run(void)
{
    SimConfig config = short_config();
    SimStats stats;
    SimSession session = {0};

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_pause(&session) == 1);
    CHECK(sim_session_stop(&session) == 1);
    CHECK(session.state == SIM_SESSION_STOPPED);
    CHECK(stats.ticks == 1);
    CHECK(stats.generated == stats.passed + stats.dropped + stats.remaining);
    CHECK(sim_session_resume(&session) == 0);
    CHECK(sim_session_step(&session) == 0);
    CHECK(sim_session_stop(&session) == 0);
}

static void test_stop_while_running_and_before_first_tick(void)
{
    SimConfig config = short_config();
    SimStats stats;
    SimSession session = {0};

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    CHECK(sim_session_stop(&session) == 1);
    CHECK(session.state == SIM_SESSION_STOPPED);
    CHECK(stats.ticks == 0);
    CHECK(stats.generated == 0 && stats.passed == 0 && stats.dropped == 0 && stats.remaining == 0);
    CHECK(stats_is_conserved(&stats) == 1);

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    CHECK(sim_session_step(&session) == 1);
    CHECK(sim_session_stop(&session) == 1);
    CHECK(session.state == SIM_SESSION_STOPPED);
    CHECK(stats.ticks == 1);
    CHECK(stats_is_conserved(&stats) == 1);
}

static void test_start_and_natural_completion(void)
{
    SimConfig config = short_config();
    SimStats stats;
    SimSession session = {0};
    int i;

    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 1);
    for (i = 0; i < config.total_ticks; i++) CHECK(sim_session_step(&session) == 1);
    CHECK(session.state == SIM_SESSION_COMPLETED);
    CHECK(stats.ticks == (unsigned int)config.total_ticks);
    CHECK(stats.generated == stats.passed + stats.dropped + stats.remaining);
    CHECK(sim_session_step(&session) == 0);
    CHECK(sim_session_pause(&session) == 0);
    CHECK(sim_session_stop(&session) == 0);
}

static void test_invalid_start_does_not_begin(void)
{
    SimConfig config = short_config();
    SimStats stats;
    SimSession session = {0};

    config.total_ticks = 0;
    CHECK(sim_session_start(&session, &config, &stats, NULL, NULL) == 0);
    CHECK(sim_session_start(NULL, &config, &stats, NULL, NULL) == 0);
}

static void test_finished_sessions_can_restart_and_repeated_runs_match(void)
{
    SimConfig config = short_config();
    SimStats first;
    SimStats second;
    SimSession session = {0};
    int i;

    CHECK(sim_session_start(&session, &config, &first, NULL, NULL) == 1);
    for (i = 0; i < config.total_ticks; i++) CHECK(sim_session_step(&session) == 1);
    CHECK(session.state == SIM_SESSION_COMPLETED);
    CHECK(sim_session_start(&session, &config, &second, NULL, NULL) == 1);
    for (i = 0; i < config.total_ticks; i++) CHECK(sim_session_step(&session) == 1);
    CHECK(session.state == SIM_SESSION_COMPLETED);
    CHECK(first.generated == second.generated);
    CHECK(first.passed == second.passed);
    CHECK(first.remaining == second.remaining);
    CHECK(first.dropped == second.dropped);
    CHECK(first.total_wait == second.total_wait);
    CHECK(stats_is_conserved(&second) == 1);
}

int main(void)
{
    TEST_RUN(test_start_pause_resume_preserves_progress);
    TEST_RUN(test_pause_resume_matches_uninterrupted_vehicle_flow);
    TEST_RUN(test_stop_while_paused_finalizes_partial_run);
    TEST_RUN(test_stop_while_running_and_before_first_tick);
    TEST_RUN(test_start_and_natural_completion);
    TEST_RUN(test_invalid_start_does_not_begin);
    TEST_RUN(test_finished_sessions_can_restart_and_repeated_runs_match);
    return TEST_SUMMARY();
}
