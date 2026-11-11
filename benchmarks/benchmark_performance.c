#include <stdio.h>
#include <time.h>

#include "simulation.h"
#include "stats.h"

#define BENCHMARK_TICKS 3600
#define BENCHMARK_RATE 7
#define EXPECTED_VEHICLES 1008

int main(void)
{
    SimConfig config;
    SimStats stats;
    clock_t start_time;
    clock_t end_time;
    double elapsed_seconds;

    sim_config_default(&config);

    config.num_directions = 4;
    config.initial_vehicles = 0;
    config.total_ticks = BENCHMARK_TICKS;
    config.green_ticks = 5;
    config.yellow_ticks = 1;
    config.discharge_per_tick = 1;
    sim_config_set_traffic_density(&config, BENCHMARK_RATE);

    if (!sim_config_validate(&config, NULL, 0)) {
        fprintf(stderr, "FAIL: Benchmark configuration is invalid.\n");
        return 1;
    }

    start_time = clock();

    if (start_time == (clock_t)-1) {
        fprintf(stderr, "FAIL: Could not start the timer.\n");
        return 1;
    }

    if (!sim_run(&config, &stats, NULL, NULL)) {
        fprintf(stderr, "FAIL: Simulation could not complete.\n");
        return 1;
    }

    end_time = clock();

    if (end_time == (clock_t)-1) {
        fprintf(stderr, "FAIL: Could not stop the timer.\n");
        return 1;
    }

    elapsed_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("\n========== PERFORMANCE BENCHMARK ==========\n");
    printf("Simulated duration : %d seconds\n", BENCHMARK_TICKS);
    printf("Directions         : %d\n", config.num_directions);
    printf("Arrival rate       : %d vehicles per 100 seconds per direction\n",
           BENCHMARK_RATE);
    printf("Expected arrivals  : %d\n", EXPECTED_VEHICLES);
    printf("Actual generated   : %lu\n", stats.generated);
    printf("Vehicles passed    : %lu\n", stats.passed);
    printf("Vehicles dropped   : %lu\n", stats.dropped);
    printf("Vehicles remaining : %lu\n", stats.remaining);
    printf("Maximum queue      : %d\n", stats.max_queue_length);
    printf("Average wait       : %.2f ticks\n", stats_average_wait(&stats));
    printf("Recorded ticks     : %u\n", stats.ticks);
    printf("Elapsed CPU time   : %.6f seconds\n", elapsed_seconds);

    if (stats.generated != EXPECTED_VEHICLES ||
        stats.dropped != 0 ||
        stats.ticks != BENCHMARK_TICKS ||
        !stats_is_conserved(&stats)) {
        printf("\nBENCHMARK RESULT: FAIL\n");
        return 1;
    }

    printf("\nBENCHMARK RESULT: PASS\n");
    return 0;
}
