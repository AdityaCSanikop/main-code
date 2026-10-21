# Traffic Management System - Simulation 

## Current status

This project is implemented and verified as a working C++ traffic simulation. The
application includes a four-lane queue system, signal-state logic, vehicle
movement rules, and automated tests covering the core behavior.

The current codebase builds cleanly with a modern C++ compiler and the unit tests
pass successfully in the repository's current state.

## Project overview

The simulation models a four-direction traffic system with North, South, East,
and West lanes. Vehicles can be added to any lane, queued in FIFO order, and
processed only when their lane is green. The system tracks signal changes,
vehicle counts, and average waiting time across the simulation.

## Features

- Four independent traffic lanes
- FIFO queue management per lane
- Duplicate-vehicle prevention and invalid-operation safeguards
- Green/yellow/red signal logic
- Vehicle exit rules enforced by the traffic manager
- Terminal-based interactive menu
- Automated test coverage for queue and vehicle behavior

## Build and run

```sh
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o traffic-simulation
./traffic-simulation
```

From the interactive menu, you can:

- Add a vehicle to a selected lane
- View the current lane states and queue contents
- Advance the simulation by one tick
- Exit the program

## Testing

The repository includes focused unit tests for the queue manager and vehicle
manager components.

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/test_queue_manager.cpp -o test_queue_manager.out
./test_queue_manager.out

g++ -std=c++17 -Wall -Wextra -pedantic tests/test_vehicle_manager.cpp -o test_vehicle_manager.out
./test_vehicle_manager.out
```

Current verified status: both test suites pass successfully.
