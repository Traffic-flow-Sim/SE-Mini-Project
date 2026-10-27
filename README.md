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
