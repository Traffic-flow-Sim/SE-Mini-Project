# Requirements Traceability Matrix (RTM)

**Project:** Queue-Based Traffic Flow Simulator
**Jira Story:** S1TMS-19
**SRS Version:** 1.0
**Status:** Local implementation and test evidence review

## 1. Status Definitions

| Status | Meaning |
|---|---|
| PASS | Implementation and corresponding local test evidence are available. |
| PARTIAL | Some aspects are implemented, but the complete SRS requirement is not demonstrated. |
| NOT VERIFIED | Evidence is insufficient to confirm the requirement. |
| NOT IMPLEMENTED | The reviewed implementation does not currently provide the requirement. |

Test filenames refer to the repository's `tests/` directory.

## 2. Functional Requirements

| SRS ID | Requirement | Implementation modules | Test case / evidence | Status |
|---|---|---|---|---|
| TFS-F-001 | Generate vehicles at a controllable arrival rate. | `vehicle_generator.c`, `simulation.c` | TC-Gen-01; `test_vehicle_generator.c`, `test_simulation.c`; 3600-tick benchmark | PASS |
| TFS-F-002 | Assign unique vehicle IDs and simulation timestamps. | `vehicle_generator.c`, `simulation.c` | TC-Gen-02; `test_vehicle_generator.c`, `test_simulation.c` | PASS |
| TFS-F-010 | Enqueue arriving vehicles when their direction is red or yellow. | `simulation.c`, `queue.c` | TC-Queue-01; `test_simulation.c` | PASS |
| TFS-F-011 | Dequeue vehicles in FIFO order when the signal is green. | `queue.c`, `simulation.c` | TC-Queue-02; `test_queue.c`, `test_queue_integrity.c`, `test_simulation.c` | PASS |
| TFS-F-012 | Enforce maximum queue capacity and an overflow policy. | `queue.c`, `simulation.c`, `stats.c` | TC-Queue-03; `test_queue_integrity.c`, `test_simulation.c` | PASS |
| TFS-F-013 | Maintain separate queues for directions and intersections. | `simulation.c`, `multi_intersection.c` | TC-Queue-04; `test_simulation.c`, `test_multi_intersection.c` | PASS |
| TFS-F-020 | Cycle red, green and yellow signals using configurable durations. | `simulation.c` | TC-Sig-01; `test_simulation.c`, `test_signal_audit.c` | PASS |
| TFS-F-021 | Maintain independent signal timing for each intersection. | `multi_intersection.c`, `simulation.c` | TC-Sig-02; `test_multi_intersection.c` | PASS |
| TFS-F-022 | Record signal-state changes with simulation timestamps. | `simulation.c`, `main.c` | TC-Sig-03; `test_signal_audit.c` | PARTIAL |
| TFS-F-030 | Record vehicle waiting times and calculate average waiting time. | `stats.c`, `simulation.c` | TC-Rep-01; `test_stats.c`, `test_simulation.c` | PASS |
| TFS-F-031 | Record maximum queue length. | `stats.c`, `simulation.c` | TC-Rep-02; `test_stats.c`, `test_simulation.c` | PASS |
| TFS-F-032 | Produce a simulation summary report. | `stats.c`, `main.c` | TC-Rep-03; `test_stats.c`, `test_simulation.c` | PASS |
| TFS-F-040 | Configure arrival rates, signal durations and simulation layout before running. | `main.c`, `multi_intersection_cli.c`, `simulation.c`, `config_file.c` | TC-Cfg-01; `test_input.c`, `test_config_file.c`, `test_multi_intersection.c` | PARTIAL |
| TFS-F-041 | Reset and rerun with new parameters without restarting the application. | `main.c`, `simulation.c` | TC-Cfg-02; `test_simulation_controls.c`; interactive load/edit/start workflow | PASS |
| TFS-F-042 | Save the current configuration to a file. | `config_file.c`, `main.c` | TC-Cfg-03; `test_config_file.c` | PASS |

### Functional requirement notes

- TFS-F-022 records signal transitions in an in-memory audit history using simulation tick values. Persistent timestamped logging is not demonstrated.
- TFS-F-040 supports configurable directional traffic rates, signal durations and intersection setup. The complete SRS interpretation of configurable lane counts needs confirmation before marking this fully passed.
- TFS-F-041 is supported by the interactive workflow, which returns to the command menu after a run and allows editing or loading settings before another run.

## 3. Non-Functional Requirements

| SRS ID | Requirement | Implementation modules | Test case / evidence | Status |
|---|---|---|---|---|
| TFS-NFR-001 | Complete a simulation equivalent to 1000 vehicles per hour without noticeable lag. | `simulation.c`, `vehicle_generator.c` | TC-NFR-01; `benchmarks/benchmark_performance.c` | PASS |
| TFS-NFR-002 | Prevent vehicle loss or duplication during queue operations. | `queue.c`, `simulation.c`, `stats.c` | TC-NFR-02; `test_queue_integrity.c`, `test_simulation.c`, `test_multi_intersection.c` | PASS |
| TFS-NFR-003 | Provide understandable configuration and report presentation. | `main.c`, `multi_intersection_cli.c`, `stats.c` | TC-NFR-03; manual console inspection and report tests | PARTIAL |
| TFS-NFR-004 | Maintain modular separation of simulator components. | All core modules | TC-NFR-04; source-module and dependency inspection | PASS |
| TFS-NFR-005 | Support Windows, Linux and macOS execution. | Source modules, `.github/workflows/ci.yml` | TC-NFR-05; configured CI matrix; remote results still required | NOT VERIFIED |

### Performance benchmark evidence

The local benchmark used:

- Simulated duration: 3600 ticks.
- Directions: 4.
- Arrival rate: 7 vehicles per 100 ticks per direction.
- Expected and actual generated vehicles: 1008.
- Vehicles passed: 1003.
- Vehicles dropped: 0.
- Vehicles remaining: 5.
- Maximum queue length: 2.
- Average waiting time: 8.20 ticks.
- Recorded CPU time: 0.003000 seconds.

The workload represents approximately 1000 generated vehicles over one simulated hour. The observed CPU time is specific to the machine used for this test.

## 4. Security Requirements

| SRS ID | Requirement | Implementation modules | Test case / evidence | Status |
|---|---|---|---|---|
| TFS-S-001 | Restrict configuration-file write access to an authorized local user or process. | `config_file.c` | TC-Sec-01; POSIX permission checks in `test_config_file.c` | PARTIAL |
| TFS-S-002 | Validate and sanitize user-supplied configuration input. | `input.c`, `config_file.c`, `simulation.c` | TC-Sec-02; `test_input.c`, `test_config_file.c`, `test_simulation.c` | PASS |
| TFS-S-003 | Keep logs and reports append-only during a simulation run. | `simulation.c`, `stats.c` | TC-Sec-03; no persistent append-only log implementation demonstrated | NOT IMPLEMENTED |
| TFS-S-004 | Create a timestamped audit entry whenever configuration changes. | `main.c`, `config_file.c` | TC-Sec-04; no configuration-change audit implementation demonstrated | NOT IMPLEMENTED |
| TFS-S-005 | Handle malformed or corrupted configuration files safely without exposing internal file paths. | `config_file.c`, `input.c` | TC-Sec-05; `test_config_file.c`, `test_input.c` | PASS |

### Security verification notes

1. On POSIX systems, `config_csv_save()` applies owner read/write permissions (`0600`). On Windows, the implementation relies on inherited directory permissions, so equivalent access restrictions have not been established by the application.
2. Strict numeric parsing, bounded input, CSV schema checks and configuration validation are covered by local tests.
3. Signal audit information is maintained in memory. This does not establish append-only persistent logging.
4. Configuration changes are not currently recorded in a separate timestamped audit log.
5. The CSV loader uses generic error messages and does not include the supplied file path in its error messages.

## 5. Test Inventory

| Test file | Main verification areas |
|---|---|
| `test_config_file.c` | CSV save/load, malformed schemas, invalid values, oversized rows, permissions and file errors |
| `test_input.c` | Numeric validation, boundaries, oversized input, EOF, CRLF and invalid arguments |
| `test_multi_intersection.c` | Independent intersections, queues, signals, controls, statistics and conservation |
| `test_queue.c` | FIFO operations, empty queue and circular queue behaviour |
| `test_queue_integrity.c` | Empty dequeue, overflow, capacity, wraparound, conservation and NULL safety |
| `test_signal_audit.c` | Signal transitions, chronological audit and session restart |
| `test_simulation.c` | Configuration, generation, queues, signals, statistics, reports and conservation |
| `test_simulation_controls.c` | Start, pause, resume, stop, completion and restart |
| `test_stats.c` | Waiting time, maximum queue, conservation and report formatting |
| `test_vehicle_generator.c` | Arrival rates, vehicle IDs, timestamps and per-direction generation |

## 6. Release Verification Items Still Outstanding

| Item | Required evidence | Current state |
|---|---|---|
| Cross-platform CI | Successful GitHub Actions runs on Ubuntu, macOS and Windows | Pending remote verification |
| Architecture documentation | Architecture description consistent with actual modules and central simulation controller | Pending |
| Final test report | Consolidated unit, integration, security and benchmark results | Pending |
| Demo script | Reproducible demonstration steps and expected outputs | Pending |
| Security gaps | Implement or formally document the limitations of TFS-S-001, S-003 and S-004 | Outstanding |
| Jira traceability | Sprint stories marked Done and linked to relevant commits | Pending |

## 7. Overall Assessment

The simulator has substantial functional and local test coverage, including queue integrity, traffic generation, signal scheduling, statistics, configuration handling and multi-intersection execution.

The performance benchmark passed locally for 1008 generated vehicles over 3600 simulated ticks.

The project is not yet ready to claim complete SRS compliance. Persistent append-only logging, timestamped configuration-change auditing, complete cross-platform CI evidence and final release documentation remain outstanding.

This matrix records the current implementation evidence and must be updated when additional requirements are implemented and verified.
