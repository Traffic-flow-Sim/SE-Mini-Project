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
