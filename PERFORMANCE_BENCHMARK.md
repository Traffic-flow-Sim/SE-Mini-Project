# Performance Benchmark Report

**Project:** Queue-Based Traffic Flow Simulator
**Jira Story:** S1TMS-19
**Benchmark source:** `benchmarks/benchmark_performance.c`
**Test status:** PASS

## 1. Objective

Verify that the simulator can process a workload equivalent to approximately 1,000 vehicle arrivals over 3,600 simulated seconds (one simulated hour), without vehicle loss due to queue overflow or simulation failure.

## 2. Test Configuration

| Parameter | Value |
|---|---:|
| Simulated duration | 3,600 seconds |
| Number of directions | 4 |
| Arrival rate per direction | 7 vehicles per 100 seconds |
| Initial vehicles | 0 |
| Green duration | 5 ticks |
| Yellow duration | 1 tick |
| Discharge rate | 1 vehicle per tick |
| Queue capacity | Default validated configuration |

The expected number of generated vehicles is:

4 directions × 7 vehicles per 100 seconds × 36 intervals = 1,008 vehicles.

This is an approximation of the SRS target of 1,000 vehicles per hour. The configured integer arrival rate produces 1,008 arrivals rather than exactly 1,000.

## 3. Build and Execution

The benchmark was compiled on Windows using GCC with strict compiler warnings treated as errors.

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. `
    benchmarks/benchmark_performance.c `
    simulation.c queue.c stats.c vehicle_generator.c `
    -o benchmark_performance.exe
```

Run the benchmark:

```powershell
.\benchmark_performance.exe
```

The benchmark uses the C `clock()` function to measure elapsed CPU time spent during the simulation run. It does not measure wall-clock elapsed time.

## 4. Verified Results

| Metric | Observed result |
|---|---:|
| Simulated duration | 3,600 seconds |
| Directions | 4 |
| Expected arrivals | 1,008 |
| Actual generated | 1,008 |
| Vehicles passed | 1,003 |
| Vehicles dropped | 0 |
| Vehicles remaining | 5 |
| Maximum queue length | 2 |
| Average waiting time | 8.20 ticks |
| Recorded ticks | 3,600 |
| Elapsed CPU time | 0.001000 seconds |
| Process exit code | 0 |

**Benchmark result: PASS**

## 5. Acceptance Checks

| Check | Expected | Observed | Status |
|---|---|---|---|
| Simulation duration | 3,600 ticks | 3,600 ticks | PASS |
| Vehicle generation | 1,008 | 1,008 | PASS |
| Queue overflow losses | 0 | 0 | PASS |
| Vehicle conservation | Generated = passed + dropped + remaining | 1,008 = 1,003 + 0 + 5 | PASS |
| Simulation completion | Successful | Exit code 0 | PASS |

## 6. Interpretation

The simulator completed all 3,600 simulated ticks and generated the expected 1,008 vehicles. No vehicles were dropped. At completion, 1,003 vehicles had passed through the intersections and five remained in queues.

The measured CPU time was 0.001000 seconds on the test machine. This is a machine- and run-specific CPU-time measurement, not a universal performance guarantee or wall-clock latency measurement.

The result provides local evidence for the SRS performance objective TFS-NFR-001. Cross-platform performance still requires execution of the benchmark in the corresponding CI environments.

## 7. Reproducibility and Limitations

- Source code is maintained in `benchmarks/benchmark_performance.c`.
- The documented GCC command rebuilds the benchmark from project source files.
- The benchmark validates generated count, zero dropped vehicles, tick count and vehicle conservation.
- CPU timing uses `clock()` and may vary across machines and runs.
- This result is from a local Windows execution. It does not establish that Ubuntu and macOS CI jobs have passed.

## 8. Conclusion

The local 3,600-tick performance benchmark passed all implemented acceptance checks. The simulator processed approximately 1,000 vehicle arrivals in a one-hour simulated workload, with zero dropped vehicles and successful vehicle-conservation validation.
