
# Traffic Flow Simulator

A queue-based traffic flow simulator implemented in C. The project models vehicle arrivals, directional queues, traffic signal transitions, and traffic statistics. It supports standard four-direction and configurable multi-intersection simulations.

## 1. Project Overview

The simulator demonstrates how queue data structures and traffic signal control can model traffic flow. Vehicles are generated according to configurable arrival rates, placed into directional queues, and processed in FIFO order when the corresponding signal permits movement.

This is an educational simulation. It does not control real-world traffic signals.

## 2. Features

- Independent vehicle queues for traffic directions.
- FIFO vehicle processing.
- Configurable vehicle arrival rates and signal durations.
- Vehicle generation and tracking.
- Queue capacity and overflow handling.
- Simulation statistics, including throughput, waiting time, and queue length.
- Multi-intersection simulation with independent intersection state.
- Pause, resume, and stop controls.
- CSV configuration loading and saving.
- Automated unit and integration tests.

## 3. Technology Requirements

- Language: C
- Compiler: GCC
- Standard: C11
- Strict compiler warnings: `-Wall -Wextra -Wpedantic -Werror`
- Target platforms in CI configuration: Ubuntu, macOS, and Windows.

No database or network connection is required.

## 4. Project Structure

| File or directory | Purpose |
|---|---|
| `main.c` | Main application and interactive configuration menu |
| `input.c`, `input.h` | Input validation and handling |
| `queue.c`, `queue.h` | Queue operations |
| `simulation.c`, `simulation.h` | Core simulation engine |
| `stats.c`, `stats.h` | Simulation statistics |
| `vehicle_generator.c`, `vehicle_generator.h` | Vehicle generation |
| `config_file.c`, `config_file.h` | CSV configuration handling |
| `multi_intersection.c`, `multi_intersection.h` | Multi-intersection simulation |
| `multi_intersection_cli.c`, `multi_intersection_cli.h` | Multi-intersection command-line interface |
| `traffic_simulation.c` | Separate basic simulation program |
| `tests/` | Automated test programs |
| `benchmarks/` | Performance benchmark source |
| `.github/workflows/ci.yml` | Continuous integration workflow |

## 5. Build the Complete Application

Open PowerShell in the project directory and run:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. main.c config_file.c input.c multi_intersection.c multi_intersection_cli.c queue.c simulation.c stats.c vehicle_generator.c -o traffic_sim.exe
```

Run the application:

```powershell
.\traffic_sim.exe
```

## 6. Standard Simulation Mode

Start the application using:

```powershell
.\traffic_sim.exe
```

The interactive configuration menu supports:

- `load` — load configuration from a CSV file.
- `edit` — edit simulation settings.
- `start` — start the simulation.
- `quit` — exit the application.

## 7. Multi-Intersection Mode

Run:

```powershell
.\traffic_sim.exe --multi
```

This mode supports multiple intersections with independent traffic queues, signal controllers, vehicle generators, and statistics. The implementation supports up to three intersections.

## 8. CSV Configuration

The application supports saving and loading simulation configurations using CSV files.

Configuration includes traffic arrival rates, signal durations, and intersection settings.

See [CONFIGURATION_CSV.md](CONFIGURATION_CSV.md) for the configuration format and usage details.

## 9. Automated Testing

The project contains ten test suites under `tests/`.

Coverage includes:

- Configuration file validation.
- Input validation.
- Multi-intersection behavior.
- Queue operations and integrity.
- Signal transition auditing.
- Simulation behavior and controls.
- Statistics calculations.
- Vehicle generation.

Compile and run an individual test using this example:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. tests/test_queue.c config_file.c input.c multi_intersection.c multi_intersection_cli.c queue.c simulation.c stats.c vehicle_generator.c -o test_queue.exe

.\test_queue.exe
```

## 10. Performance Benchmark

The benchmark simulates 3,600 ticks and checks vehicle generation, processing, queue behavior, and simulation statistics.

Compile:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. benchmarks/benchmark_performance.c simulation.c queue.c stats.c vehicle_generator.c -o benchmark_performance.exe
```

Run:

```powershell
.\benchmark_performance.exe
```

See [PERFORMANCE_BENCHMARK.md](PERFORMANCE_BENCHMARK.md) for the benchmark methodology and recorded results.

## 11. Continuous Integration

The GitHub Actions workflow is located at `.github/workflows/ci.yml`.

It is configured to build and test the project on Ubuntu, macOS, and Windows. It includes application builds, smoke tests, configuration checks, automated test suites, and a Linux memory-safety job.

Remote CI results must be verified in GitHub Actions after the latest commits are pushed. A configured workflow alone does not establish that all platform jobs have passed.

## 12. Requirements and Verification

Project documentation:

- [REQUIREMENTS_TRACEABILITY_MATRIX.md](REQUIREMENTS_TRACEABILITY_MATRIX.md) — maps SRS requirements to implementation modules and test cases.
- [ARCHITECTURE.md](ARCHITECTURE.md) — describes the layered architecture and simulation controller.
- [TEST_REPORT.md](TEST_REPORT.md) — documents test coverage, results, and verification limitations.
- [DEMO_SCRIPT.md](DEMO_SCRIPT.md) — provides reproducible demonstration steps.
- [PERFORMANCE_BENCHMARK.md](PERFORMANCE_BENCHMARK.md) — describes performance testing and results.

## 13. Known Limitations

- Signal transition audit records are maintained in memory and use simulation ticks; persistent timestamped audit logging is not implemented.
- Persistent append-only logs and timestamped configuration-change auditing are not implemented.
- Configuration file permissions on Windows rely on inherited directory permissions.
- Cross-platform CI must be confirmed using the latest GitHub Actions run.
- Some requirements are partially implemented or require additional verification, as documented in the requirements traceability matrix.

## 14. Project Scope

This project is intended for academic demonstration and analysis of queue-based traffic flow simulation. It provides configurable traffic generation, signal control, queue management, multi-intersection support, and reporting without interfacing with physical traffic infrastructure.
