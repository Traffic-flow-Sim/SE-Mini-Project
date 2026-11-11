# Software Verification and Test Report

**Project:** Queue-Based Traffic Flow Simulator
**Jira Story:** S1TMS-19
**SRS Version:** 1.0
**Test date:** 11 November 2026
**Test environment:** Windows 11 Pro 64-bit (Build 26200)
**Compiler:** MinGW.org GCC 6.3.0-1
**Overall local test result:** PASS

## 1. Purpose

This report documents the verification performed for the Queue-Based Traffic Flow Simulator. It covers unit and integration tests, input validation, queue integrity, multi-intersection operation, performance benchmarking, security-related checks and outstanding release verification.

The report distinguishes locally executed tests from checks configured in GitHub Actions but not yet remotely verified.

## 2. Test Environment and Build Configuration

| Item | Value |
|---|---|
| Operating system | Microsoft Windows 11 Pro |
| OS version | 10.0.26200 |
| Architecture | 64-bit |
| Compiler | MinGW.org GCC 6.3.0-1 |
| Language standard | C11 |
| Warning flags | -Wall -Wextra -Wpedantic -Werror |
| Include path | -I. |
| Test source directory | tests/ |

Each test source was compiled with the project implementation modules:

- config_file.c
- input.c
- multi_intersection.c
- multi_intersection_cli.c
- queue.c
- simulation.c
- stats.c
- vehicle_generator.c

The test executables were freshly compiled before execution. Existing generated executables were not used as the sole evidence of successful tests.

## 3. Unit and Integration Test Results

All eleven test suites compiled successfully and executed with exit code zero.

| Test suite | Verification area | Reported checks | Result |
|---|---|---:|---|
| test_audit_log | Persistent signal/configuration logging, append behaviour, sanitization and error handling | 41 | PASS |
| test_config_file | CSV configuration save/load, validation and error handling | 33 | PASS |
| test_input | Numeric parsing, boundaries, oversized input, EOF and CRLF | 66 | PASS |
| test_multi_intersection | Independent intersections, signals, queues, controls and statistics | 120 | PASS |
| test_queue | FIFO queue operations and empty queue behaviour | Not reported | PASS |
| test_queue_integrity | Capacity, overflow, wraparound, conservation and NULL safety | 1,653 | PASS |
| test_signal_audit | Signal transitions, chronological ordering and restart | 69 | PASS |
| test_simulation | Configuration, generation, signals, queues, reports and conservation | 1,955 | PASS |
| test_simulation_controls | Start, pause, resume, stop and restart | 108 | PASS |
| test_stats | Waiting time, maximum queue, conservation and report formatting | 36 | PASS |
| test_vehicle_generator | Arrival rates, unique IDs and directional generation | 9,056 | PASS |

### 3.1 Test summary

- Total test suites: 11
- Successful builds: 11
- Successful executions: 11
- Build failures: 0
- Execution failures: 0
- Individually reported checks passed: 13,137

The queue test suite did not print an individual check count. Therefore, 13,137 is the sum of the reported checks from the other ten suites, not a total including an assumed count for the queue suite.

### 3.2 Important verified behaviours

The local test execution verified:

1. FIFO queue ordering and safe dequeue from an empty queue.
2. Queue capacity enforcement and overflow accounting.
3. Vehicle conservation and unique vehicle identifiers.
4. Independent traffic queues and signal timing across intersections.
5. Correct green, yellow and red signal transitions.
6. Start, pause, resume, stop and restart behaviour.
7. Configuration file round trips and rejection of malformed configuration.
8. Numeric boundary validation and oversized input rejection.
9. Statistics, average waiting time and report formatting.
10. Safe handling of invalid arguments and NULL pointers in covered modules.

## 4. Performance Benchmark

### 4.1 Objective

Verify that the simulator can complete a workload equivalent to approximately 1,000 generated vehicles over 3,600 simulated seconds.

### 4.2 Configuration

| Parameter | Value |
|---|---:|
| Simulated duration | 3,600 ticks |
| Directions | 4 |
| Arrival rate | 7 vehicles per 100 ticks per direction |
| Initial vehicles | 0 |
| Green duration | 5 ticks |
| Yellow duration | 1 tick |
| Discharge per tick | 1 vehicle |
| Expected arrivals | 1,008 |

Expected arrivals:

4 directions × 7 vehicles × 36 intervals = 1,008 vehicles.

### 4.3 Observed results

| Measurement | Result |
|---|---:|
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

The generated vehicle count matched the expected count, no vehicles were dropped, the simulation completed all 3,600 ticks, and the vehicle conservation check passed.

The timing value is CPU time measured using C `clock()`, not wall-clock time. It is specific to the local machine and execution.

Detailed configuration and reproducibility instructions are in `PERFORMANCE_BENCHMARK.md`. The benchmark implementation is in `benchmarks/benchmark_performance.c`.

## 5. Security and Robustness Verification

| Area | Evidence | Assessment |
|---|---|---|
| Input validation | Numeric parsing, range checks and oversized-input tests | PASS |
| Queue safety | Empty dequeue, overflow, wraparound and NULL safety tests | PASS |
| Configuration validation | Strict CSV schema and value validation | PASS |
| Malformed configuration handling | Rejected files preserve the existing configuration | PASS |
| Configuration-file permissions | POSIX owner-only permissions are implemented; Windows relies on inherited directory permissions | PARTIAL |
| Persistent append-mode audit logging | Signal transitions and configuration changes are timestamped and appended to `traffic_audit.log`; external tampering or deletion is not prevented | PARTIAL |
| Timestamped configuration audit | Interactive configuration changes, successful CSV loads and multi-intersection configuration creation are recorded with timestamps and intersection IDs | PASS |

### 5.1 Security limitations

1. Configuration saving applies owner read/write permissions (0600) on POSIX systems. Windows access control relies on inherited directory permissions.
2. User input and configuration values are validated using bounded parsing and explicit range checks.
3. Signal-state and configuration audit entries are persisted in `traffic_audit.log` using append mode, with wall-clock timestamps and relevant simulation details.
4. Append mode does not make the log tamper-proof. External processes or users with filesystem permissions may still modify or delete the log.
5. The configuration loader uses generic error messages and does not expose the supplied file path in those messages.

These limitations must be considered when assessing complete SRS security compliance.

## 6. Continuous Integration and Cross-Platform Verification

The repository contains `.github/workflows/ci.yml`.

The workflow is configured to run on:

- Ubuntu
- macOS
- Windows

Configured checks include:

1. Strict C11 compilation with warnings treated as errors.
2. Integrated application build.
3. Basic single-direction simulation build.
4. Application smoke test.
5. Simulation console-control checks.
6. Traffic-density tests.
7. Configuration save/load and overwrite tests.
8. Invalid, oversized and incomplete input tests.
9. All eleven test suites, including persistent audit-log tests.
10. Ubuntu AddressSanitizer and UndefinedBehaviorSanitizer checks.
11. Ubuntu Valgrind memory checks.

**Verification status:** The workflow is present and configured, but successful remote GitHub Actions results on all three operating systems have not yet been established for this release.

The sanitizer and Valgrind jobs are also configured in CI; they are not claimed as locally executed tests in this report.

## 7. Requirements Traceability Assessment

The detailed requirement-to-module and requirement-to-test mapping is maintained in `REQUIREMENTS_TRACEABILITY_MATRIX.md`.

Current assessment:

| Requirement category | PASS | PARTIAL | NOT VERIFIED | NOT IMPLEMENTED |
|---|---:|---:|---:|---:|
| Functional | 14 | 1 | 0 | 0 |
| Non-functional | 3 | 1 | 1 | 0 |
| Security | 3 | 2 | 0 | 0 |

Important outstanding requirements include:

- TFS-F-040: the complete SRS interpretation of configurable lane counts requires confirmation.
- TFS-NFR-003: the usability and understandability of configuration and reports require further assessment.
- TFS-NFR-005: cross-platform execution needs successful remote CI evidence.
- TFS-S-001: equivalent configuration-file access restrictions on Windows are not established by the application.
- TFS-S-003: persistent append-mode logging is implemented, but protection against external modification or deletion is not established.

TFS-F-022 signal-state logging and TFS-S-004 timestamped configuration auditing have been implemented and locally verified.

Passing unit tests do not automatically establish complete SRS compliance. Refer to the RTM for the evidence and status of each requirement.

## 8. Release Readiness and Outstanding Work

| Deliverable or activity | Current state |
|---|---|
| Requirements traceability matrix | Created and committed |
| Layered architecture documentation | Created and committed |
| Performance benchmark source | Created and committed |
| Performance benchmark report | Created and committed |
| Local unit and integration tests | 11 of 11 suites passed |
| Consolidated test report | Created |
| Demo script | Created and committed |
| README build, run and verification instructions | Updated and committed |
| Remote Ubuntu/macOS/Windows CI verification | Pending |
| Remote sanitizer and Valgrind verification | Pending |
| Jira story completion and commit linkage | Pending confirmation |
| Outstanding security requirements | S-003 and S-001 remain partial; limitations documented |

## 9. Conclusion

The local Windows verification completed successfully. All eleven freshly compiled test suites passed, with zero build or execution failures and 13,137 individually reported checks passing across ten suites. The queue test suite passed but did not report an individual check count.

The performance benchmark also passed for 1,008 generated vehicles over 3,600 simulated seconds, with zero dropped vehicles and a successful vehicle conservation check.

Persistent timestamped signal-state logging and configuration-change auditing have been implemented and locally verified. However, append-mode logging does not prevent external modification or deletion, so TFS-S-003 remains partially satisfied.

The simulator demonstrates substantial functional coverage across queue management, vehicle generation, signal scheduling, configuration, statistics and multi-intersection operation.

Complete release acceptance and full SRS compliance cannot yet be claimed. Cross-platform GitHub Actions results remain pending, and TFS-F-040, TFS-NFR-003, TFS-NFR-005, TFS-S-001 and TFS-S-003 require further verification or improvement. Jira story completion and commit linkage also remain to be confirmed.

This report records the evidence available on 11 November 2026 and should be updated when the outstanding release checks are completed.