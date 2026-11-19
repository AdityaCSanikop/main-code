# Traffic Management System Simulation

A C++ traffic simulation that models a four-direction signal system with lane-specific FIFO queues, safe vehicle entry/exit rules, and a simple interactive terminal interface.

## Overview

The project simulates traffic flow for four lanes: North, South, East, and West. Vehicles are assigned to a lane, queued in arrival order, and only leave the queue when their lane is green. The system tracks signal transitions, queue length, and average waiting time, making it suitable for demonstrating queue-management logic and traffic control rules.

## Features

- Four independent traffic lanes
- FIFO queue management per lane
- Duplicate vehicle prevention and validation of blank vehicle IDs and types
- Green, yellow, and red signal state handling
- Vehicle exit enforcement through the traffic manager
- Terminal-based menu for adding vehicles, viewing status, and advancing time
- Congestion levels based on queue length and traffic statistics, including average waiting time
- Automated tests covering queue and vehicle manager behavior

## Project structure

```text
.
├── main.cpp                     # Core simulation logic and interactive interface
├── README.md                   # Project overview and usage instructions
├── LICENSE                     # Project license
├── tests/
│   ├── test_queue_manager.cpp  # Queue manager validation tests
│   ├── test_vehicle_manager.cpp # Vehicle manager validation tests
│   └── test_performance.cpp    # Performance and end-to-end checks
├── web-ui/index.html           # Browser UI for the simulation (see "Web UI" below)
├── sprint1 outputs/            # Sprint 1 deliverables and artifacts
├── sprint2 outputs/            # Sprint 2 deliverables and artifacts
├── test_queue_manager.out      # Built queue test binary
├── test_vehicle_manager.out    # Built vehicle test binary
└── .vscode/                    # Editor settings
```

## Build and run

Compile the simulation:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o traffic-simulation
```

Run it:

```sh
./traffic-simulation
```

From the interactive menu, you can:

- Add a vehicle to a specific lane
- View the current lane states and queue contents
- Advance the simulation by one tick
- Exit the application

## Behavior rules

- Vehicles may only join a valid lane.
- Vehicle IDs must be unique across the entire simulation.
- Vehicle IDs and types cannot be empty or whitespace-only.
- Queue operations follow FIFO order.
- A vehicle can leave its lane only when the lane signal is green.
- Red or yellow lanes block vehicle exits and keep the queue unchanged.
- A lane with more than five waiting vehicles is reported as highly congested.
- The status report shows each lane's signal, queue length and contents, plus
  total vehicles added and processed and average waiting time for processed vehicles.

## Testing

The repository includes unit tests for the queue and vehicle manager components.

Build and run the queue tests:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/test_queue_manager.cpp -o test_queue_manager.out
./test_queue_manager.out
```

Build and run the vehicle tests:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/test_vehicle_manager.cpp -o test_vehicle_manager.out
./test_vehicle_manager.out
```

Current verified status: the test suites pass successfully with the current implementation.

## Web UI

`web-ui/index.html` is a single-file browser front end for the simulation. It needs no build step or install.

- **Open it:** double-click `web-ui/index.html`, or serve the folder (`python -m http.server 8765 --directory web-ui`) and visit `http://localhost:8765`.
- **Use it:** add vehicles to a lane, advance one tick (or press Space), or auto-run. The intersection, lane cards, statistics and event log update as the simulation runs. The congestion threshold (default 5) can be changed in the statistics card.
- **Self-test:** click *Run self-tests* at the bottom of the page, or open `index.html?test`, to run browser versions of test cases TC-01 to TC-12 from the test plan.

The UI mirrors the C++ core instead of calling it. Its JavaScript `QueueManager`, `VehicleManager`, signal cycle and validation follow `main.cpp`: the same green duration (3 ticks), N→S→E→W order, exit-on-green rule, unique vehicle IDs, LOW/MEDIUM/HIGH levels and the same log and error messages. On top of that, the UI adds a configurable congestion threshold (congested when queue length > threshold), per-lane statistics, an integrity check (added = processed + waiting) and length limits on input (ID ≤ 12, type ≤ 20 characters). If you change behaviour in `main.cpp`, update `web-ui/index.html` to match.

## Notes

This project is implemented as a C++ simulation focused on traffic-flow logic, queue integrity, and signal-controlled vehicle movement. It is intended to demonstrate safe state transitions, validation checks, and basic simulation behavior in a compact, testable design.
