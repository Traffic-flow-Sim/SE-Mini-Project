# Project Demonstration Script



**Project:** Queue-Based Traffic Flow Simulator

**Jira Story:** S1TMS-19

**SRS Version:** 1.0

**Suggested presentation duration:** 10–15 minutes

**Platform:** Windows 11, PowerShell, GCC



## 1. Demonstration Objectives



Demonstrate the following project capabilities:



1\. Vehicle generation and FIFO queue management.

2\. Traffic signal transitions and vehicle discharge.

3\. Interactive configuration and CSV save/load.

4\. Start, pause, resume and stop controls.

5\. Independent operation of multiple intersections.

6\. Traffic statistics and vehicle conservation.

7\. Performance and automated test results.



The application is an educational simulation. It does not control real traffic signals.



## 2. Preparation



Open PowerShell in the project root directory.



Compile the complete application using strict C11 compiler warnings:



```powershell

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. main.c input.c queue.c simulation.c stats.c vehicle\_generator.c config\_file.c multi\_intersection.c multi\_intersection\_cli.c -o traffic\_sim.exe

```



Launch standard mode:



```powershell

.\\traffic\_sim.exe

```



Launch multi-intersection mode:



```powershell

.\\traffic\_sim.exe --multi

```



Expected result: compilation completes without errors or warnings.



## 3. Demonstration A — Standard Simulation Mode



### 3.1 Prepare a demonstration configuration



Create a temporary CSV file:



```powershell

$demoConfig = Join-Path $env:TEMP "traffic\_demo\_config.csv"



@'

directions,north\_density,south\_density,east\_density,west\_density,green\_ticks,yellow\_ticks,queue\_capacity,initial\_vehicles,total\_ticks

4,0,0,0,0,2,1,100,2,8

'@ | Set-Content -Path $demoConfig -Encoding ascii



Get-Content $demoConfig

Write-Host $demoConfig

```



The configuration uses:



| Setting | Value |

|---|---:|

| Directions | 4 |

| Initial vehicles per direction | 2 |

| Arrival density per direction | 0 |

| Green duration | 2 ticks |

| Yellow duration | 1 tick |

| Queue capacity | 100 |

| Simulation length | 8 ticks |



Zero arrival density keeps this short demonstration focused on initial vehicles and signal-controlled queue discharge.



### 3.2 Load the configuration



Start the application:



```powershell

.\\traffic\_sim.exe

```



At the configuration prompt, enter:



```text

load

```



When prompted for the CSV path, enter the path displayed by:



```powershell

Write-Host $demoConfig

```



Expected message:



```text

Configuration loaded.

```



Explain:



"The application validates the complete CSV configuration before replacing the current settings. An invalid configuration is rejected, and the existing settings are preserved."



### 3.3 Demonstrate simulation controls



At the configuration menu, enter:



```text

start

```



The application displays live traffic information, including:



- Simulation tick.

- Direction and signal state.

- Remaining signal duration.

- Queue length.

- Vehicles arriving, passing and being dropped.

- Total vehicles passed and waiting.



At the simulation command prompt, demonstrate:



```text

step

pause

resume

step

stop

```



Expected observations:



- `step` advances the simulation by one tick.

- `pause` suspends simulation processing.

- `resume` continues the paused session.

- `stop` safely stops the active session.



The exact tick and vehicle counts depend on the commands entered and the signal phase when stopping.



After stopping, inspect:



- Signal Change Audit Trail.

- Final statistics report.

- Generated, passed, dropped and remaining vehicles.

- Average waiting time and maximum queue length.

- Vehicle conservation result.



At the save prompt, enter `no`.



At the next configuration menu, enter:



```text

quit

```



### 3.4 Explain CSV configuration support



The CSV format uses a fixed header and one data row. It validates field count, direction-specific density values, numeric ranges and malformed input.



The complete format and validation rules are documented in `CONFIGURATION\_CSV.md`.



## 4. Demonstration B — Multi-Intersection Mode



### 4.1 Launch multi-intersection mode



Run:



```powershell

.\\traffic\_sim.exe --multi

```



Enter the following sample inputs.



**Number of intersections:**



```text

2

```



**Intersection 1:**



| Prompt | Input |

|---|---:|

| Initial vehicles per direction | 2 |

| Green signal duration | 2 |

| Traffic density | 10 |



**Intersection 2:**



| Prompt | Input |

|---|---:|

| Initial vehicles per direction | 3 |

| Green signal duration | 3 |

| Traffic density | 20 |



Both intersections are configured independently. Their default yellow duration and other default settings are retained.



### 4.2 Start and demonstrate independent controls



At the start prompt, enter:



```text

start

```



Use the following commands at the `multi>` prompt:



```text

pause 1

step

resume 1

step

stop 2

quit

```



Expected observations:



1\. Intersection 1 pauses without requiring Intersection 2 to pause.

2\. A simulation step advances eligible running intersections.

3\. Intersection 1 resumes independently.

4\. Intersection 2 can be stopped independently.

5\. `quit` safely stops any remaining active intersections.



### 4.3 Inspect final reports



The application displays:



- Separate final reports for each intersection.

- Signal audit history for each intersection.

- Combined traffic statistics.

- Vehicle conservation status for the overall simulation.

- Individual vehicle conservation results.



Explain:



"Each intersection maintains its own simulation session, queues, signal timing, vehicle generation and statistics. The manager coordinates the intersections while preserving their independent state."



## 5. Demonstration C — Performance Benchmark



The benchmark verifies the simulator against approximately 1,000 generated vehicles over 3,600 simulated seconds.



Compile:



```powershell

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. benchmarks/benchmark\_performance.c simulation.c queue.c stats.c vehicle\_generator.c -o benchmark\_performance.exe

```



Run:



```powershell

.\\benchmark\_performance.exe

```



Previously verified local results:



| Measurement | Result |

|---|---:|

| Simulated duration | 3,600 ticks |

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

| Result | PASS |



Explain:



"The benchmark completes all 3,600 simulated ticks, generates the expected 1,008 vehicles, drops none, and passes the vehicle conservation check."



The recorded timing is CPU time measured using C `clock()`, not wall-clock time. It is machine- and execution-dependent. Rerunning may produce a different timing measurement.



Refer to `PERFORMANCE\_BENCHMARK.md` for the full methodology and recorded output.



## 6. Demonstration D — Automated Tests



The project has ten C test suites under `tests/`.



To compile and execute all suites from PowerShell:



```powershell

$common = @(

&#x20;   "config\_file.c",

&#x20;   "input.c",

&#x20;   "multi\_intersection.c",

&#x20;   "multi\_intersection\_cli.c",

&#x20;   "queue.c",

&#x20;   "simulation.c",

&#x20;   "stats.c",

&#x20;   "vehicle\_generator.c"

)



$failures = 0



foreach ($test in Get-ChildItem .\\tests\\test\_\*.c) {

&#x20;   $exe = Join-Path $env:TEMP "$($test.BaseName)\_demo.exe"



&#x20;   Write-Host "`n========== $($test.BaseName) =========="



&#x20;   \& gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. $test.FullName @common -o $exe



&#x20;   if ($LASTEXITCODE -ne 0) {

&#x20;       Write-Host "BUILD FAILED"

&#x20;       $failures++

&#x20;       continue

&#x20;   }



&#x20;   \& $exe



&#x20;   if ($LASTEXITCODE -ne 0) {

&#x20;       Write-Host "TEST FAILED"

&#x20;       $failures++

&#x20;   } else {

&#x20;       Write-Host "TEST PASSED"

&#x20;   }

}



Write-Host "`n========== AUTOMATED TEST SUMMARY =========="

if ($failures -eq 0) {

&#x20;   Write-Host "PASS: All test suites built and executed successfully."

} else {

&#x20;   Write-Host "FAIL: $failures test suite(s) failed."

}

```



Previously verified local results:



| Test suite | Result |

|---|---|

| test\_config\_file | PASS |

| test\_input | PASS |

| test\_multi\_intersection | PASS |

| test\_queue | PASS |

| test\_queue\_integrity | PASS |

| test\_signal\_audit | PASS |

| test\_simulation | PASS |

| test\_simulation\_controls | PASS |

| test\_stats | PASS |

| test\_vehicle\_generator | PASS |



All ten suites previously compiled and executed successfully. Nine suites reported a combined 13,096 individual checks; the queue suite did not print an individual count.



## 7. Security and Robustness Points to Explain



Demonstrate or describe the evidence for:



- Rejection of invalid numeric input and out-of-range configuration.

- Rejection of oversized input.

- Safe empty queue dequeue and queue overflow handling.

- FIFO ordering and vehicle conservation.

- Validation of malformed CSV files.

- Preservation of the existing configuration after a failed load.

- Generic configuration-loading errors.



Be transparent about current limitations:



- Persistent append-only logging is not implemented.

- Timestamped configuration-change auditing is not implemented.

- Signal audit history is maintained in memory using simulation ticks.

- Windows configuration-file permissions rely on inherited directory permissions.



Do not claim these outstanding security requirements are implemented.



## 8. Documentation and Release Evidence



Show the following project documents:



| Document | Purpose |

|---|---|

| `README.md` | Project overview and build/run instructions |

| `REQUIREMENTS\_TRACEABILITY\_MATRIX.md` | SRS requirement-to-module and test mapping |

| `ARCHITECTURE.md` | Layered architecture and central simulation controller |

| `CONFIGURATION\_CSV.md` | CSV configuration schema and validation |

| `PERFORMANCE\_BENCHMARK.md` | Benchmark methodology and results |

| `TEST\_REPORT.md` | Consolidated local verification and outstanding work |

| `DEMO\_SCRIPT.md` | Reproducible project demonstration |



The repository also contains `.github/workflows/ci.yml`, configured for Ubuntu, macOS and Windows.



Do not describe the cross-platform workflow as green until successful remote GitHub Actions runs have been verified.



## 9. Suggested Presentation Sequence



| Approximate time | Activity |

|---|---|

| 0–1 min | Introduce the problem, purpose and scope |

| 1–3 min | Show standard-mode configuration and CSV loading |

| 3–5 min | Demonstrate signals, queues and simulation controls |

| 5–8 min | Demonstrate two independent intersections |

| 8–9 min | Show final statistics and vehicle conservation |

| 9–10 min | Run or explain the performance benchmark |

| 10–12 min | Show automated tests, RTM and architecture |

| 12–15 min | Explain security limitations and release status |



## 10. Final Demonstration Checklist



- [ ] Complete application compiles with strict GCC flags.

- [ ] Standard mode launches successfully.

- [ ] Valid CSV configuration loads successfully.

- [ ] Live signal and queue information is displayed.

- [ ] Step, pause, resume and stop are demonstrated.

- [ ] Final statistics and signal audit are displayed.

- [ ] Multi-intersection mode launches with two intersections.

- [ ] Independent pause, resume and stop commands are demonstrated.

- [ ] Individual and combined reports are displayed.

- [ ] Vehicle conservation passes.

- [ ] Performance benchmark completes 3,600 ticks.

- [ ] Automated test suites pass.

- [ ] RTM and architecture documentation are shown.

- [ ] Outstanding security and cross-platform verification gaps are disclosed.



## 11. Closing Statement



"The Queue-Based Traffic Flow Simulator demonstrates FIFO-based vehicle queues, configurable traffic signals, vehicle generation, simulation controls, statistics, CSV configuration and independently managed intersections. Local Windows verification and the performance benchmark have passed. The requirements traceability matrix and test report document the remaining gaps, including persistent security auditing and remote cross-platform CI verification."
