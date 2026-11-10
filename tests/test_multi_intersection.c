#include "multi_intersection.h"
#include "test_util.h"

#include <string.h>

static SimConfig make_config(int green_ticks, int yellow_ticks, int total_ticks,
                             int initial_vehicles, int density)
{
    SimConfig config;

    sim_config_default(&config);
    config.num_directions = 2;
    config.green_ticks = green_ticks;
    config.yellow_ticks = yellow_ticks;
    config.total_ticks = total_ticks;
    config.initial_vehicles = initial_vehicles;
    sim_config_set_traffic_density(&config, density);

    return config;
}

static void run_manager_to_completion(SimMultiIntersection *manager)
{
    int steps = 0;

    while (sim_multi_step(manager)) {
        steps++;
        CHECK(steps <= SIM_MAX_TICKS);
    }
}

static void test_valid_and_invalid_intersection_counts(void)
{
    SimMultiIntersection manager;
    SimConfig configs[SIM_MAX_INTERSECTIONS];
    int i;

    for (i = 0; i < SIM_MAX_INTERSECTIONS; i++) {
        configs[i] = make_config(2, 1, 5, 1, 0);
    }

    CHECK(sim_multi_init(&manager, 1, configs) == 1);
    CHECK(manager.count == 1);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(manager.count == 2);

    CHECK(sim_multi_init(&manager, 3, configs) == 1);
    CHECK(manager.count == 3);

    CHECK(sim_multi_init(&manager, 0, configs) == 0);
    CHECK(sim_multi_init(&manager, 4, configs) == 0);
    CHECK(sim_multi_init(&manager, -1, configs) == 0);
    CHECK(sim_multi_init(NULL, 2, configs) == 0);
    CHECK(sim_multi_init(&manager, 2, NULL) == 0);

    configs[1].total_ticks = 0;
    CHECK(sim_multi_init(&manager, 2, configs) == 0);

    CHECK(sim_multi_get(&manager, 0) == NULL);
    CHECK(sim_multi_get(&manager, 4) == NULL);
    CHECK(sim_multi_get(NULL, 1) == NULL);
}

static void test_intersections_have_independent_resources(void)
{
    SimConfig configs[2];
    SimMultiIntersection manager;

    configs[0] = make_config(1, 1, 5, 2, 0);
    configs[1] = make_config(3, 2, 7, 4, 0);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    CHECK(manager.intersections[0].session.stats ==
          &manager.intersections[0].stats);
    CHECK(manager.intersections[1].session.stats ==
          &manager.intersections[1].stats);

    CHECK(&manager.intersections[0].session.queues[0] !=
          &manager.intersections[1].session.queues[0]);

    CHECK(&manager.intersections[0].session.generator !=
          &manager.intersections[1].session.generator);

    CHECK(manager.intersections[0].session.config.green_ticks == 1);
    CHECK(manager.intersections[1].session.config.green_ticks == 3);

    CHECK(manager.intersections[0].session.config.initial_vehicles == 2);
    CHECK(manager.intersections[1].session.config.initial_vehicles == 4);

    CHECK(sim_multi_step(&manager) == 1);

    CHECK(manager.intersections[0].session.next_tick == 1);
    CHECK(manager.intersections[1].session.next_tick == 1);

    CHECK(manager.intersections[0].stats.generated == 4);
    CHECK(manager.intersections[1].stats.generated == 8);

    CHECK(manager.intersections[0].session.generator.next_id == 5);
    CHECK(manager.intersections[1].session.generator.next_id == 9);
}

static void test_signal_timings_are_independent(void)
{
    SimConfig configs[2];
    SimMultiIntersection manager;

    configs[0] = make_config(1, 1, 5, 0, 0);
    configs[1] = make_config(3, 2, 5, 0, 0);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    CHECK(sim_signal_at(&manager.intersections[0].session.config, 0, 1)
          != sim_signal_at(&manager.intersections[1].session.config, 0, 1));

    CHECK(sim_multi_step(&manager) == 1);
    CHECK(sim_multi_step(&manager) == 1);

    CHECK(manager.intersections[0].session.next_tick == 2);
    CHECK(manager.intersections[1].session.next_tick == 2);

    CHECK(manager.intersections[0].session.config.green_ticks == 1);
    CHECK(manager.intersections[1].session.config.green_ticks == 3);
}

static void test_pause_and_resume_affect_only_selected_intersection(void)
{
    SimConfig configs[2];
    SimMultiIntersection manager;
    unsigned int first_tick;
    unsigned int second_tick;

    configs[0] = make_config(2, 1, 6, 2, 0);
    configs[1] = make_config(3, 1, 6, 2, 0);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    CHECK(sim_multi_step(&manager) == 1);

    CHECK(sim_multi_pause(&manager, 1) == 1);
    CHECK(manager.intersections[0].session.state == SIM_SESSION_PAUSED);
    CHECK(manager.intersections[1].session.state == SIM_SESSION_RUNNING);

    first_tick = manager.intersections[0].session.next_tick;
    second_tick = manager.intersections[1].session.next_tick;

    CHECK(sim_multi_step(&manager) == 1);

    CHECK(manager.intersections[0].session.next_tick == first_tick);
    CHECK(manager.intersections[1].session.next_tick == second_tick + 1);

    CHECK(sim_multi_resume(&manager, 1) == 1);
    CHECK(manager.intersections[0].session.state == SIM_SESSION_RUNNING);

    CHECK(sim_multi_pause(&manager, 99) == 0);
    CHECK(sim_multi_resume(&manager, 99) == 0);
    CHECK(sim_multi_stop(&manager, 99) == 0);

    CHECK(sim_multi_stop(&manager, 1) == 1);
    CHECK(manager.intersections[0].session.state == SIM_SESSION_STOPPED);
    CHECK(manager.intersections[1].session.state == SIM_SESSION_RUNNING);

    CHECK(sim_multi_step(&manager) == 1);
    CHECK(manager.intersections[0].session.next_tick == first_tick);
    CHECK(manager.intersections[1].session.next_tick == second_tick + 2);
}

static void test_individual_and_combined_statistics(void)
{
    SimConfig configs[2];
    SimMultiIntersection manager;
    const SimStats *combined;
    unsigned long expected_generated;
    unsigned long expected_passed;
    unsigned long expected_dropped;
    unsigned long expected_remaining;
    int expected_max_queue;

    configs[0] = make_config(1, 1, 3, 2, 0);
    configs[1] = make_config(2, 1, 5, 3, 0);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    run_manager_to_completion(&manager);

    CHECK(manager.intersections[0].session.state == SIM_SESSION_COMPLETED);
    CHECK(manager.intersections[1].session.state == SIM_SESSION_COMPLETED);

    CHECK(stats_is_conserved(&manager.intersections[0].stats) == 1);
    CHECK(stats_is_conserved(&manager.intersections[1].stats) == 1);

    expected_generated =
        manager.intersections[0].stats.generated +
        manager.intersections[1].stats.generated;

    expected_passed =
        manager.intersections[0].stats.passed +
        manager.intersections[1].stats.passed;

    expected_dropped =
        manager.intersections[0].stats.dropped +
        manager.intersections[1].stats.dropped;

    expected_remaining =
        manager.intersections[0].stats.remaining +
        manager.intersections[1].stats.remaining;

    expected_max_queue =
        manager.intersections[0].stats.max_queue_length >
        manager.intersections[1].stats.max_queue_length
        ? manager.intersections[0].stats.max_queue_length
        : manager.intersections[1].stats.max_queue_length;

    combined = sim_multi_get_combined_stats(&manager);

    CHECK(combined != NULL);
    CHECK(combined->generated == expected_generated);
    CHECK(combined->passed == expected_passed);
    CHECK(combined->dropped == expected_dropped);
    CHECK(combined->remaining == expected_remaining);
    CHECK(combined->max_queue_length == expected_max_queue);
    CHECK(combined->ticks == 5);

    CHECK(stats_is_conserved(combined) == 1);
    CHECK(sim_multi_is_conserved(&manager) == 1);
}

static void test_vehicle_conservation_during_and_after_run(void)
{
    SimConfig configs[2];
    SimMultiIntersection manager;

    configs[0] = make_config(1, 1, 8, 100, 100);
    configs[1] = make_config(2, 1, 8, 100, 100);

    CHECK(sim_multi_init(&manager, 2, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    CHECK(sim_multi_is_conserved(&manager) == 1);

    CHECK(sim_multi_step(&manager) == 1);
    CHECK(sim_multi_is_conserved(&manager) == 1);

    CHECK(sim_multi_step(&manager) == 1);
    CHECK(sim_multi_is_conserved(&manager) == 1);

    run_manager_to_completion(&manager);

    CHECK(sim_multi_is_conserved(&manager) == 1);
    CHECK(stats_is_conserved(&manager.intersections[0].stats) == 1);
    CHECK(stats_is_conserved(&manager.intersections[1].stats) == 1);
    CHECK(stats_is_conserved(&manager.combined_stats) == 1);
}

static void test_one_intersection_matches_existing_simulation(void)
{
    SimConfig config = make_config(2, 1, 6, 3, 10);
    SimConfig configs[1];
    SimStats baseline;
    SimMultiIntersection manager;

    configs[0] = config;

    CHECK(sim_run(&config, &baseline, NULL, NULL) == 1);
    CHECK(sim_multi_init(&manager, 1, configs) == 1);
    CHECK(sim_multi_start(&manager) == 1);

    run_manager_to_completion(&manager);

    CHECK(manager.intersections[0].session.state == SIM_SESSION_COMPLETED);

    CHECK(manager.intersections[0].stats.generated == baseline.generated);
    CHECK(manager.intersections[0].stats.passed == baseline.passed);
    CHECK(manager.intersections[0].stats.dropped == baseline.dropped);
    CHECK(manager.intersections[0].stats.remaining == baseline.remaining);
    CHECK(manager.intersections[0].stats.total_wait == baseline.total_wait);
    CHECK(manager.intersections[0].stats.max_queue_length ==
          baseline.max_queue_length);
    CHECK(manager.intersections[0].stats.ticks == baseline.ticks);

    CHECK(manager.combined_stats.generated == baseline.generated);
    CHECK(manager.combined_stats.passed == baseline.passed);
    CHECK(manager.combined_stats.dropped == baseline.dropped);
    CHECK(manager.combined_stats.remaining == baseline.remaining);
    CHECK(sim_multi_is_conserved(&manager) == 1);
}

int main(void)
{
    TEST_RUN(test_valid_and_invalid_intersection_counts);
    TEST_RUN(test_intersections_have_independent_resources);
    TEST_RUN(test_signal_timings_are_independent);
    TEST_RUN(test_pause_and_resume_affect_only_selected_intersection);
    TEST_RUN(test_individual_and_combined_statistics);
    TEST_RUN(test_vehicle_conservation_during_and_after_run);
    TEST_RUN(test_one_intersection_matches_existing_simulation);

    return TEST_SUMMARY();
}
