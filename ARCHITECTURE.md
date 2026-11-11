# System Architecture

**Project:** Queue-Based Traffic Flow Simulator
**Jira Story:** S1TMS-19
**Architecture style:** Layered, modular, procedural C architecture
**Primary language:** C (C11)

## 1. Architecture Overview

The Queue-Based Traffic Flow Simulator uses a layered architecture. Each module has a defined responsibility, separating console interaction, configuration handling, simulation control, queue management, vehicle generation and statistics.

The central simulation controller is implemented through `SimSession` and its associated functions in `simulation.c`. It manages the lifecycle and execution of a single-intersection simulation.

For multi-intersection execution, `SimMultiIntersection` manages up to three independent intersections. Each intersection owns its own configuration, simulation session, queues, vehicle generator and statistics.

The application is a standalone console program. It does not require a network connection, external database or specialized hardware.

## 2. High-Level Architecture

```text
+------------------------------------------------------+
|                 Presentation Layer                  |
|                                                      |
|             main.c                                   |
|             multi_intersection_cli.c                 |
|                                                      |
|       Interactive menu, commands and display         |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|              Configuration and Input Layer          |
|                                                      |
|             input.c / input.h                       |
|             config_file.c / config_file.h           |
|                                                      |
|       Input validation, CSV load and CSV save        |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|                Simulation Control Layer              |
|                                                      |
|             simulation.c / simulation.h             |
|             multi_intersection.c / multi_intersection.h |
|                                                      |
|       SimSession: central single-intersection        |
|       simulation controller                         |
|       SimMultiIntersection: manages independent     |
|       intersection sessions                         |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|                  Core Simulation Layer               |
|                                                      |
|             queue.c / queue.h                       |
|             vehicle_generator.c / vehicle_generator.h |
|                                                      |
|       FIFO vehicle queues and deterministic          |
|       vehicle generation                            |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|                Statistics and Reporting Layer       |
|                                                      |
|             stats.c / stats.h                       |
|                                                      |
|       Vehicle counts, waiting times, queue lengths   |
|       and simulation summary reports                 |
+------------------------------------------------------+
```

The diagram represents logical responsibilities and major dependencies. Statistics are shared by the simulation and presentation workflows rather than forming a strictly one-way dependency from the core layer.

## 3. Architectural Layers and Modules

### 3.1 Presentation Layer

**Files:** `main.c`, `multi_intersection_cli.c`, `multi_intersection_cli.h`

Responsibilities:
- Display the interactive application menu.
- Accept user commands and simulation settings.
- Start, pause, resume and stop simulations.
- Display signal states, queue information and statistics.
- Provide the multi-intersection configuration and control interface.

`main.c` coordinates the single-intersection workflow and provides the main application entry point. `multi_intersection_cli.c` provides the user interface for multi-intersection execution.

### 3.2 Configuration and Input Layer

**Files:** `input.c`, `input.h`, `config_file.c`, `config_file.h`

Responsibilities:
- Read bounded console input.
- Trim and parse user-entered values.
- Validate numeric input and reject invalid values.
- Load and save simulation settings using CSV files.
- Validate CSV headers, fields, values and row structure before accepting configuration data.

The CSV configuration module validates a candidate configuration before replacing the active configuration. On POSIX systems, saved configuration files are restricted to owner read/write permissions where supported. On Windows, file access relies on inherited directory permissions.

### 3.3 Simulation Control Layer

**Files:** `simulation.c`, `simulation.h`, `multi_intersection.c`, `multi_intersection.h`

This layer contains the central simulation controller and coordinates simulation execution.

**SimSession**
- Stores the simulation configuration and current state.
- Owns the direction queues and vehicle generator for the session.
- Manages simulation start, pause, resume, stop and tick execution.
- Coordinates signal scheduling and vehicle movement.
- Records signal transitions in an in-memory audit history.
- Updates statistics as the simulation progresses.

The `sim_run()` function provides a convenient complete-run interface using the simulation session functionality.

**SimMultiIntersection**
- Manages between one and three intersections.
- Maintains an independent session and configuration for each intersection.
- Advances the configured intersections during multi-intersection execution.
- Supports individual intersection controls.
- Maintains combined statistics and vehicle-conservation checks.

### 3.4 Core Simulation Layer

**Files:** `queue.c`, `queue.h`, `vehicle_generator.c`, `vehicle_generator.h`

**Queue module**
- Implements bounded FIFO vehicle queues.
- Maintains queue front and size information.
- Enqueues arriving vehicles and dequeues vehicles in FIFO order.
- Tracks dropped vehicles when a queue reaches its configured capacity.
- Handles invalid or empty queue operations safely.

**Vehicle generator module**
- Generates vehicles according to configured arrival rates.
- Uses a deterministic rate-accumulation mechanism.
- Assigns vehicle identifiers and arrival timestamps.
- Maintains separate generation state for each direction within an intersection.

### 3.5 Statistics and Reporting Layer

**Files:** `stats.c`, `stats.h`

Responsibilities:
- Track generated, passed, dropped and remaining vehicles.
- Calculate total and average waiting time.
- Track maximum queue length.
- Record the number of processed simulation ticks.
- Check vehicle conservation.
- Format and print simulation summary reports.

Statistics are maintained per simulation and can also be combined for multi-intersection reporting.

## 4. Single-Intersection Execution Flow

1. The presentation layer obtains the simulation settings.
2. The input and configuration modules validate the settings.
3. A `SimSession` is initialized with the selected configuration.
4. The simulation controller initializes queues and vehicle generation.
5. Each simulation tick generates eligible vehicle arrivals.
6. Vehicles are added to their direction queues.
7. The signal controller determines the active signal state.
8. Vehicles are dequeued from the eligible green direction according to the configured discharge rate.
9. Statistics and signal-transition audit entries are updated.
10. The simulation continues until completion or until the user stops it.
11. The reporting module displays the final statistics.

## 5. Multi-Intersection Execution Flow

1. The user selects the number of intersections, within the supported range of one to three.
2. The multi-intersection interface obtains and validates each intersection's configuration.
3. `SimMultiIntersection` initializes an independent simulation session for every intersection.
4. Each intersection maintains its own queues, signal schedule, generator and statistics.
5. The manager advances the active intersection sessions.
6. Individual intersections can be paused, resumed or stopped independently.
7. Per-intersection statistics are maintained.
8. Combined statistics and vehicle-conservation information can be displayed.

## 6. Important Data Structures

| Structure | Module | Purpose |
|---|---|---|
| `SimConfig` | `simulation.h` | Stores simulation settings such as directions, arrival rates, timings and duration. |
| `SimSession` | `simulation.h` | Holds the state and resources for one simulation session. |
| `SimTickInfo` | `simulation.h` | Provides information about a simulation tick. |
| `SimSignalAuditEntry` | `simulation.h` | Records a signal transition with its simulation tick and direction. |
| `Vehicle` | `queue.h` | Stores a vehicle identifier and arrival tick. |
| `VehicleQueue` | `queue.h` | Implements a bounded FIFO queue. |
| `VehicleGenerator` | `vehicle_generator.h` | Maintains vehicle generation state and identifiers. |
| `SimStats` | `stats.h` | Stores simulation counts and waiting-time statistics. |
| `SimIntersection` | `multi_intersection.h` | Holds the configuration, session and statistics for one intersection. |
| `SimMultiIntersection` | `multi_intersection.h` | Manages multiple independent intersections and combined statistics. |

## 7. Timing and Signal Behaviour

The simulation uses discrete time ticks. One tick represents one simulated second.

- Signal timing is controlled by the configured green and yellow durations.
- The signal controller cycles through the configured directions.
- A direction that is not currently green remains red, except during its own transition period.
- Vehicle arrivals are processed before departures within a tick.
- Vehicles leave a queue only when its direction is eligible for green-signal discharge.
- Signal transitions are recorded with simulation tick values in the session's in-memory audit history.

The current audit history is not a persistent, wall-clock-timestamped log.

## 8. Configuration and Data Flow

Simulation settings include:
- Number of directions or intersections, depending on the selected mode.
- Vehicle arrival rates or traffic density.
- Initial vehicle count.
- Queue capacity.
- Green and yellow signal durations.
- Vehicle discharge rate.
- Total simulation duration.

Configuration can be entered through the console or loaded from a CSV file. The configuration module validates the data before the simulation uses it.

The project uses local files for configuration. It does not use a database or network communication.

## 9. Error Handling and Data Integrity

The implementation includes:
- Bounded console input and strict numeric parsing.
- Validation of simulation configuration values.
- CSV schema and field validation.
- Rejection of malformed or oversized input.
- Bounded queue operations and overflow accounting.
- Safe handling of empty dequeue operations.
- Checks for invalid pointers in relevant queue operations.
- Vehicle-conservation checks.
- Simulation session state validation.

The test suite includes queue, input, configuration, vehicle-generation, signal, statistics, simulation-control and multi-intersection tests. The requirements traceability matrix records the available test evidence and remaining limitations.

## 10. Build Components

The integrated application is built from the following source modules:

- `main.c`
- `input.c`
- `config_file.c`
- `queue.c`
- `simulation.c`
- `vehicle_generator.c`
- `stats.c`
- `multi_intersection.c`
- `multi_intersection_cli.c`

`traffic_simulation.c` is a separate legacy/basic simulation entry point used by the relevant standalone build and test workflow. It is not the entry point for the integrated multi-intersection application.

The project uses standard C facilities and GCC. The CI workflow is configured for Ubuntu, macOS and Windows; successful remote execution must be confirmed from the CI results.

## 11. Known Limitations

- The application is a simulator and does not control real traffic signals.
- The application is standalone and does not use a database or network service.
- Signal-transition history is maintained in memory and uses simulation ticks rather than wall-clock timestamps.
- Persistent append-only run logs are not implemented.
- Configuration-change audit entries with timestamps are not implemented.
- On Windows, configuration-file permissions depend on inherited directory access controls.
- Cross-platform CI status must be established using actual workflow execution results.

These limitations are reflected in the requirements traceability matrix and should not be interpreted as completed security or release requirements.

## 12. Conclusion

The project follows a layered and modular architecture with `SimSession` acting as the central controller for a single-intersection simulation and `SimMultiIntersection` coordinating multiple independent sessions.

The separation of presentation, configuration, simulation control, queue management, vehicle generation and reporting supports maintainability, testing and future extension.
