// Performance tests for NFR1 / TC-09 (issue #9).
// Build and run from the repo root:
//   g++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/test_performance.cpp -o test_performance.out
//   ./test_performance.out
//
// TC-09 sets no response-time limit (the SRS does not define one), so these
// tests pass on completion and correctness: no crash, hang, lost vehicle or
// duplicated vehicle. Elapsed times are printed for the test plan's
// "Actual Result" column.

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

// Pull in the classes from main.cpp without its main() function.
#define TMS_UNIT_TEST
#include "../main.cpp"

namespace {

int failures = 0;

void check(bool condition, const std::string& description) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << description << '\n';

    if (!condition) {
        ++failures;
    }
}

double millisecondsSince(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

// Component level: a large number of vehicles through the Vehicle Manager and
// Queue Manager, cycling GREEN across the lanes until every queue is empty.
void testComponentStress() {
    const std::size_t laneCount = 4;
    const std::size_t vehicleCount = 200000;

    std::cout << "NFR1 component stress (" << vehicleCount << " vehicles, "
              << laneCount << " lanes)\n";

    QueueManager queues(laneCount);
    VehicleManager vehicles(queues);

    const auto start = std::chrono::steady_clock::now();

    std::size_t added = 0;
    for (std::size_t i = 0; i < vehicleCount; ++i) {
        if (vehicles.addVehicle(i % laneCount, "V" + std::to_string(i), "car", 0)) {
            ++added;
        }
    }

    // Each lane remembers the last vehicle number that left it, to confirm
    // FIFO order (vehicle numbers in a lane only ever increase).
    std::vector<long long> lastLeft(laneCount, -1);
    std::unordered_set<std::string> departedIds;
    bool fifoKept = true;
    bool redBlocked = true;
    std::size_t departed = 0;
    Vehicle out;

    while (departed < added) {
        for (std::size_t lane = 0; lane < laneCount; ++lane) {
            // Red lanes must never release a vehicle.
            if (vehicles.moveVehicle(lane, SignalState::Red, out) != MoveResult::BlockedBySignal) {
                redBlocked = false;
            }

            // Three vehicles per green, matching GREEN_DURATION_TICKS.
            for (int tick = 0; tick < 3; ++tick) {
                if (vehicles.moveVehicle(lane, SignalState::Green, out) != MoveResult::Moved) {
                    break;
                }

                const long long number = std::stoll(out.id.substr(1));
                fifoKept = fifoKept && number > lastLeft[lane];
                lastLeft[lane] = number;
                departedIds.insert(out.id);
                ++departed;
            }
        }
    }

    const double ms = millisecondsSince(start);

    std::size_t stillQueued = 0;
    for (std::size_t lane = 0; lane < laneCount; ++lane) {
        stillQueued += queues.getQueueLength(lane);
    }

    check(added == vehicleCount, "all vehicles were added");
    check(departed == vehicleCount, "all vehicles left their lanes");
    check(departedIds.size() == vehicleCount, "no vehicle left twice (no duplicates)");
    check(stillQueued == 0, "every queue is empty at the end (no lost vehicles)");
    check(fifoKept, "every lane kept FIFO order");
    check(redBlocked, "no vehicle ever left on RED");
    std::cout << "        completed in " << ms << " ms\n";
}

// End to end (TC-09 steps): drive the real TrafficSimulation menu loop with
// scripted input — add vehicles to all lanes, advance through many signal
// cycles, and request statistics along the way.
void testSimulationWorkload() {
    const std::size_t vehicleCount = 20000;
    const std::size_t laneCount = 4;

    std::cout << "TC-09 full simulation workload (" << vehicleCount << " vehicles)\n";

    std::ostringstream script;

    for (std::size_t i = 0; i < vehicleCount; ++i) {
        script << "1\n" << (i % laneCount) + 1 << "\nPV" << i << "\ncar\n";
    }

    // Each lane is GREEN for 3 ticks then YELLOW for 1, so one full cycle of
    // 4 lanes takes 16 ticks and moves at most 12 vehicles. Allow extra ticks.
    const std::size_t ticks = (vehicleCount / 12 + 1) * 16 + 64;

    for (std::size_t t = 0; t < ticks; ++t) {
        script << "3\n";

        if (t % 2000 == 0) {
            script << "2\n"; // statistics / status display
        }
    }

    script << "2\n4\n"; // final status, then exit

    std::istringstream input(script.str());
    std::ostringstream output;

    std::streambuf* const originalIn = std::cin.rdbuf(input.rdbuf());
    std::streambuf* const originalOut = std::cout.rdbuf(output.rdbuf());

    const auto start = std::chrono::steady_clock::now();
    {
        TrafficSimulation simulation;
        simulation.run();
    }
    const double ms = millisecondsSince(start);

    std::cin.rdbuf(originalIn);
    std::cout.rdbuf(originalOut);

    const std::string log = output.str();
    const std::string expectedTotals =
        "Total vehicles added     : " + std::to_string(vehicleCount) +
        "\n" +
        "Total vehicles processed : " + std::to_string(vehicleCount);

    const std::size_t lastStatus = log.rfind("========================================\n"
        "          TRAFFIC STATUS REPORT");
    const std::string finalStatus =
        lastStatus == std::string::npos ? "" : log.substr(lastStatus);

    check(log.find("Simulation ended.") != std::string::npos,
          "simulation ran every command and exited normally (no hang)");
    check(log.find("could not be queued") == std::string::npos &&
              log.find("was not added") == std::string::npos,
          "every vehicle was accepted");
    check(finalStatus.find(expectedTotals) != std::string::npos,
          "final statistics show all vehicles added and processed");
    check(finalStatus.find("Vehicles     : 0") != std::string::npos &&
              finalStatus.find("Queue        : Empty") != std::string::npos,
          "every lane is empty at the end");
    check(log.find("YELLOW") != std::string::npos &&
              log.find("RED") != std::string::npos,
          "signals cycled through YELLOW and RED during the run");
    std::cout << "        " << ticks << " ticks completed in " << ms << " ms\n";
}

} // namespace

int main() {
    testComponentStress();
    testSimulationWorkload();

    std::cout << '\n'
              << (failures == 0 ? "All tests passed.\n" : "Some tests failed.\n");
    return failures == 0 ? 0 : 1;
}
