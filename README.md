# Traffic Management System (Simulation)

A terminal-based C++ simulation of vehicle queues and traffic signals. Add
vehicles to a North, South, East, or West lane, inspect queue lengths, and
advance the simulation to let the green lane process its next vehicle.

## Build and run

```sh
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o traffic-simulation
./traffic-simulation
```

Choose **Add vehicle**, select a lane, and enter a unique vehicle ID and type.
The vehicle joins that lane's queue and waits until its signal is green.
**Advance simulation** processes at most one vehicle from the current green
lane, advances the tick, and moves the green signal to the next lane.
**View traffic status** shows the signal state, queued vehicles, queue lengths,
and overall vehicle counts and average waiting time.
