# Traffic Management Simulation

## Overview
This project implements independent vehicle queues and traffic signal phases for a four-direction intersection.

## My Contribution: S1TMS-6
Implemented separate queues and signal phases for North, South, East, and West directions.

## Features
- Four independent FIFO queues.
- Separate traffic signal status for each direction.
- Only the direction with the green signal can dequeue a vehicle.
- Queue lengths are displayed before and after each signal phase.
- The simulation cycles through all four directions.

## Technologies
- Programming language: C
- Compiler: GCC

## How to Compile and Run
Compile using:

```bash
gcc -Wall -Wextra -std=c11 main.c -o traffic_sim
```

Run on Windows PowerShell:

```powershell
.\traffic_sim.exe
```

## Testing
1. Verify that all four directions have independent queues.
2. Verify that only the green direction's queue decreases.
3. Verify that red-direction queues remain unchanged.
4. Verify that queue lengths are displayed before and after each phase.

## Jira Task
S1TMS-6 — Separate queues and signal phases per direction


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

## Configurable Traffic Density and Signal Timing

### Overview
The simulation allows users to configure traffic density and green signal duration before each run.

### Configuration Limits
- **Traffic density:** 1–100 vehicles per direction.
- **Green signal duration:** 1–120 seconds per direction.

### Input Validation
The program validates user input and displays an error message when the input is:
- Zero or negative.
- Non-numeric.
- Outside the permitted range.

The program continues requesting input until a valid value is entered.

### Simulation Behaviour
- Separate vehicle queues are maintained for North, South, East, and West.
- Only the direction with the green signal can process a vehicle.
- Each signal phase remains active for the configured duration.
- The configured settings apply to the current simulation run.

### Testing
- Minimum valid values: density `1`, duration `1`.
- Maximum density: `100`.
- Maximum signal duration: `120`.
- Density values `0`, `-1`, and `101` are rejected.
- Signal duration values `0`, `-1`, and `121` are rejected.
- Non-numeric input is rejected.
- The simulation was verified to process vehicles in the correct direction during each signal phase.

---

## S1TMS-18: Multi-Intersection Traffic Simulation

### Overview
The simulation supports running 1 to 3 independent traffic intersections in one execution. Each intersection maintains its own traffic configuration, vehicle queues, signal controller, vehicle generator, and statistics.

### Features
- Configure between 1 and 3 intersections.
- Set initial vehicles, green signal duration, and traffic density independently for each intersection.
- Maintain independent queues and signal timing for every intersection.
- Pause, resume, or stop an individual intersection without affecting the others.
- Display live traffic status separately for each intersection.
- Generate individual final reports and combined traffic statistics.
- Verify vehicle conservation for each intersection and the overall simulation.

### Compilation

Compile the complete application using GCC:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. main.c input.c queue.c simulation.c stats.c vehicle_generator.c multi_intersection.c multi_intersection_cli.c -o traffic_sim.exe
```

### Running Multi-Intersection Mode

On Windows PowerShell:

```powershell
.\traffic_sim.exe --multi
```

The program asks for:
- Number of intersections: `1-3`.
- Initial vehicles per direction: `1-100`.
- Green signal duration: `1-120`.
- Traffic density: `0–100` vehicles per direction per 100 seconds.

Each intersection receives its own configuration.

### Interactive Commands

After configuration, enter `start` to begin.

| Command | Description |
|---|---|
| `step` | Advance every running intersection by one simulation tick. |
| `pause N` | Pause intersection N. |
| `resume N` | Resume intersection N. |
| `stop N` | Stop intersection N. |
| `quit` | Stop all active intersections and exit the interactive run. |

Replace `N` with the intersection ID (`1`, `2`, or `3`, depending on the configured count).

### Statistics and Vehicle Conservation

The program displays:
- Individual generated, passed, dropped, and remaining vehicle counts.
- Individual intersection reports and signal audit histories.
- Combined statistics across all configured intersections.
- Vehicle conservation checks.

The conservation condition is:

`Generated = Passed + Dropped + Still Queued`

### Testing

The multi-intersection implementation includes tests for:
1. Valid and invalid intersection counts.
2. Independent queues, sessions, and vehicle generators.
3. Independent signal timing.
4. Individual pause, resume, and stop operations.
5. Individual and combined statistics.
6. Vehicle conservation during and after execution.
7. Regression equivalence between a one-intersection manager run and the existing simulation.

The complete test suite is compiled with strict GCC warnings enabled (`-Wall -Wextra -Wpedantic -Werror`).

### Jira Task

S1TMS-18 - Support multiple intersections in one simulation run.