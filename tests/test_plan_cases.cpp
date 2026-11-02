// System tests for every test case in the Test Plan (TC-01 to TC-12), issue #27.
//
// Each test drives the real TrafficSimulation menu loop with scripted
// keyboard input and checks the console output, as described in the Test
// Plan's procedure (§4.5): build, execute, observe, compare, record.
//
// Build and run from the repo root:
//   g++ -std=c++14 -O2 -Wall -Wextra tests/test_plan_cases.cpp -o test_plan_cases.exe
//   .\test_plan_cases.exe          (Windows)
//   ./test_plan_cases.exe          (Linux / macOS)

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Pull in the classes from main.cpp without its main() function.
#define TMS_UNIT_TEST
#include "../main.cpp"

namespace {

// ---------------------------------------------------------------------------
// Test bookkeeping
// ---------------------------------------------------------------------------

int caseFailures = 0;
std::vector<std::pair<std::string, bool>> summary;

void check(bool condition, const std::string& description) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << description << '\n';

    if (!condition) {
        ++caseFailures;
    }
}

void beginCase(const std::string& title) {
    caseFailures = 0;
    std::cout << '\n' << title << '\n';
}

void endCase(const std::string& id) {
    summary.emplace_back(id, caseFailures == 0);
}

// ---------------------------------------------------------------------------
// Running the simulation with scripted input
// ---------------------------------------------------------------------------

std::string runSimulation(const std::string& script) {
    std::istringstream input(script + "4\n"); // always exit at the end
    std::ostringstream output;

    std::streambuf* const originalIn = std::cin.rdbuf(input.rdbuf());
    std::streambuf* const originalOut = std::cout.rdbuf(output.rdbuf());

    {
        TrafficSimulation simulation;
        simulation.run();
    }

    std::cin.rdbuf(originalIn);
    std::cout.rdbuf(originalOut);
    return output.str();
}

// Menu commands, written the way a user would type them.
std::string add(int lane, const std::string& id, const std::string& type = "car") {
    return "1\n" + std::to_string(lane) + "\n" + id + "\n" + type + "\n";
}

std::string addRaw(const std::string& laneInput) {
    return "1\n" + laneInput + "\n";
}

std::string status() { return "2\n"; }

std::string advance(int ticks = 1) {
    std::string s;
    for (int i = 0; i < ticks; ++i) {
        s += "3\n";
    }
    return s;
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

std::size_t countOf(const std::string& text, const std::string& part) {
    std::size_t count = 0;
    for (std::size_t pos = text.find(part); pos != std::string::npos;
         pos = text.find(part, pos + part.size())) {
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Reading the "TRAFFIC STATUS REPORT" screens back into data
// ---------------------------------------------------------------------------

struct LaneStatus {
    std::string signal;
    std::string level;
    std::size_t vehicles = 0;
    std::string queue;
};

struct StatusReport {
    std::size_t tick = 0;
    std::map<std::string, LaneStatus> lanes;
    std::size_t added = 0;
    std::size_t processed = 0;
    double averageWait = 0.0;
};

std::string valueAfter(const std::string& line, const std::string& label) {
    return line.substr(label.size());
}

std::vector<StatusReport> parseReports(const std::string& log) {
    std::vector<StatusReport> reports;
    std::istringstream lines(log);
    std::string line;
    std::string lane;

    while (std::getline(lines, line)) {
        if (contains(line, "TRAFFIC STATUS REPORT")) {
            reports.emplace_back();
        } else if (reports.empty()) {
            continue;
        } else if (line.rfind("Simulation Tick: ", 0) == 0) {
            reports.back().tick = std::stoul(valueAfter(line, "Simulation Tick: "));
        } else if (line.rfind("LANE: ", 0) == 0) {
            lane = valueAfter(line, "LANE: ");
        } else if (line.rfind("Signal State : ", 0) == 0) {
            reports.back().lanes[lane].signal = valueAfter(line, "Signal State : ");
        } else if (line.rfind("Traffic Level: ", 0) == 0) {
            reports.back().lanes[lane].level = valueAfter(line, "Traffic Level: ");
        } else if (line.rfind("Vehicles     : ", 0) == 0) {
            reports.back().lanes[lane].vehicles =
                std::stoul(valueAfter(line, "Vehicles     : "));
        } else if (line.rfind("Queue        : ", 0) == 0) {
            reports.back().lanes[lane].queue = valueAfter(line, "Queue        : ");
        } else if (line.rfind("Total vehicles added     : ", 0) == 0) {
            reports.back().added = std::stoul(valueAfter(line, "Total vehicles added     : "));
        } else if (line.rfind("Total vehicles processed : ", 0) == 0) {
            reports.back().processed =
                std::stoul(valueAfter(line, "Total vehicles processed : "));
        } else if (line.rfind("Average waiting time     : ", 0) == 0) {
            reports.back().averageWait =
                std::stod(valueAfter(line, "Average waiting time     : "));
        }
    }

    return reports;
}

const std::vector<std::string> laneNames = {"North", "South", "East", "West"};

std::size_t totalQueued(const StatusReport& report) {
    std::size_t total = 0;
    for (const auto& lane : report.lanes) {
        total += lane.second.vehicles;
    }
    return total;
}

// ---------------------------------------------------------------------------
// TC-01 — FR1, SR1: add a vehicle to a valid lane
// ---------------------------------------------------------------------------
void tc01() {
    beginCase("TC-01 (FR1, SR1) add a vehicle with valid input");
    const std::string log = runSimulation(add(2, "KA01", "car") + status());
    const auto reports = parseReports(log);

    check(contains(log, "Vehicle KA01 added to the South lane queue."),
          "success confirmation is displayed");
    check(reports.size() == 1, "status report is shown");
    if (reports.size() == 1) {
        const StatusReport& r = reports[0];
        check(r.lanes.at("South").vehicles == 1 &&
                  r.lanes.at("South").queue == "KA01 (car)",
              "KA01 is queued in the South lane");
        check(totalQueued(r) == 1, "no other lane was changed");
        check(r.added == 1, "statistics count 1 vehicle added");
    }
    endCase("TC-01");
}

// ---------------------------------------------------------------------------
// TC-02 — FR2, FR5: per-lane FIFO queue, enter and leave
// ---------------------------------------------------------------------------
void tc02() {
    beginCase("TC-02 (FR2, FR5) per-lane FIFO order");
    const std::string log = runSimulation(add(1, "V1") + add(1, "V2") + add(1, "V3") +
                                          status() + advance() + status());
    const auto reports = parseReports(log);

    check(reports.size() == 2, "two status reports are shown");
    if (reports.size() == 2) {
        check(reports[0].lanes.at("North").vehicles == 3,
              "queue length is 3 before removal");
        check(reports[0].lanes.at("North").queue == "V1 (car) -> V2 (car) -> V3 (car)",
              "queue order is V1 -> V2 -> V3");
        check(contains(log, "vehicle V1 (car) moved from the North lane"),
              "V1 is dequeued first");
        check(reports[1].lanes.at("North").vehicles == 2 &&
                  reports[1].lanes.at("North").queue == "V2 (car) -> V3 (car)",
              "queue length becomes 2 with V2 at the front");
    }
    endCase("TC-02");
}

// ---------------------------------------------------------------------------
// TC-03 — FR3, FR4: signal states and cycle
// ---------------------------------------------------------------------------
void tc03() {
    beginCase("TC-03 (FR3, FR4) signal cycle Red / Yellow / Green");
    const int ticks = 20;
    std::string script = status();
    for (int t = 0; t < ticks; ++t) {
        script += advance() + status();
    }

    const auto reports = parseReports(runSimulation(script));
    check(reports.size() == static_cast<std::size_t>(ticks + 1),
          "a status report was captured after every tick");

    bool onlyValidStates = true;
    bool oneActiveLane = true;
    bool followsCycle = true;

    for (std::size_t k = 0; k < reports.size(); ++k) {
        // Expected: each lane is GREEN for 3 ticks, then YELLOW for 1 tick,
        // then RED while the next lane (North -> South -> East -> West) runs.
        const std::size_t activeLane = (k / 4) % 4;
        const std::string activeState = (k % 4 == 3) ? "YELLOW" : "GREEN";
        int nonRed = 0;

        for (std::size_t i = 0; i < laneNames.size(); ++i) {
            const std::string& state = reports[k].lanes.at(laneNames[i]).signal;

            if (state != "RED" && state != "YELLOW" && state != "GREEN") {
                onlyValidStates = false;
            }
            if (state != "RED") {
                ++nonRed;
            }

            const std::string expected = (i == activeLane) ? activeState : "RED";
            if (state != expected) {
                followsCycle = false;
            }
        }

        if (nonRed != 1) {
            oneActiveLane = false;
        }
    }

    check(onlyValidStates, "only RED, YELLOW and GREEN are ever shown");
    check(oneActiveLane, "exactly one lane is GREEN or YELLOW at any time");
    check(followsCycle,
          "each lane goes GREEN (3 ticks) -> YELLOW (1 tick) -> RED, in order N, S, E, W");
    endCase("TC-03");
}

// ---------------------------------------------------------------------------
// TC-04 — FR5, FR3: vehicle leaves only when its lane is GREEN
// ---------------------------------------------------------------------------
void tc04() {
    beginCase("TC-04 (FR5, FR3) vehicle waits on RED/YELLOW, leaves on GREEN");
    // South is RED at the start. North is GREEN for ticks 1-3 and YELLOW
    // after tick 3; South turns GREEN at tick 4 and moves a vehicle at tick 5.
    std::string script = add(2, "S1");
    for (int t = 0; t < 5; ++t) {
        script += advance() + status();
    }
    const std::string log = runSimulation(script);
    const auto reports = parseReports(log);

    check(reports.size() == 5, "a status report was captured after every tick");
    if (reports.size() == 5) {
        bool waitedOnRed = true;
        for (int k = 0; k < 3; ++k) {
            waitedOnRed = waitedOnRed && reports[k].lanes.at("South").signal == "RED" &&
                          reports[k].lanes.at("South").vehicles == 1;
        }
        check(waitedOnRed, "S1 stays queued while South is RED (ticks 1-3)");
        check(reports[2].lanes.at("North").signal == "YELLOW" &&
                  reports[2].lanes.at("South").vehicles == 1,
              "S1 stays queued while North is YELLOW");
        check(reports[3].lanes.at("South").signal == "GREEN",
              "South turns GREEN on the normal cycle (tick 4)");
        check(reports[4].lanes.at("South").vehicles == 0 &&
                  contains(log, "Tick 5: vehicle S1 (car) moved from the South lane"),
              "S1 leaves once South is GREEN (tick 5)");
        check(countOf(log, "moved from the South lane") == 1,
              "S1 left exactly once");
    }
    endCase("TC-04");
}

// ---------------------------------------------------------------------------
// TC-05 — FR6, NFR2: display checklist
// ---------------------------------------------------------------------------
void tc05() {
    beginCase("TC-05 (FR6, NFR2) status display checklist");
    const std::string log =
        runSimulation(add(1, "N1", "car") + add(3, "E1", "bus") + add(3, "E2", "truck") +
                      advance() + status());
    const auto reports = parseReports(log);

    check(reports.size() == 1, "status report is shown");
    if (reports.size() == 1) {
        const StatusReport& r = reports[0];
        bool everyLaneListed = true;
        for (const std::string& name : laneNames) {
            everyLaneListed = everyLaneListed && r.lanes.count(name) == 1 &&
                              !r.lanes.at(name).signal.empty();
        }
        check(everyLaneListed, "every lane shows its name and signal state");
        check(r.tick == 1, "current simulation tick is shown");
        check(r.lanes.at("North").vehicles == 0 && r.lanes.at("North").queue == "Empty",
              "North count (0) matches state after N1 left");
        check(r.lanes.at("East").vehicles == 2 &&
                  r.lanes.at("East").queue == "E1 (bus) -> E2 (truck)",
              "East count (2) and contents match, with vehicle types");
        check(r.lanes.at("North").signal == "GREEN" && r.lanes.at("East").signal == "RED",
              "signal states match the simulation");
        check(contains(log, "TRAFFIC STATISTICS") && r.added == 3 && r.processed == 1,
              "statistics section is shown with correct totals");
    }
    endCase("TC-05");
}

// ---------------------------------------------------------------------------
// TC-06 / TC-07 — FR7: congestion indicator
//
// The congestion indicator is shown as "Traffic Level" in the status report:
// LOW (0-2), MEDIUM (3-5), HIGH (more than 5). HIGH means congested, so the
// congestion threshold is 5 vehicles.
// ---------------------------------------------------------------------------
void tc06() {
    beginCase("TC-06 (FR7) lane above threshold is congested");
    std::string script;
    for (int i = 1; i <= 5; ++i) {
        script += add(2, "C" + std::to_string(i));
    }
    script += status() + add(2, "C6") + status();

    const auto reports = parseReports(runSimulation(script));
    check(reports.size() == 2, "two status reports are shown");
    if (reports.size() == 2) {
        check(reports[0].lanes.at("South").vehicles == 5 &&
                  reports[0].lanes.at("South").level != "HIGH",
              "exactly at threshold (5) is not congested");
        check(reports[1].lanes.at("South").vehicles == 6 &&
                  reports[1].lanes.at("South").level == "HIGH",
              "one above threshold (6) is congested (HIGH)");
        check(reports[1].lanes.at("North").level == "LOW",
              "other lanes are not reported congested");
    }
    endCase("TC-06");
}

void tc07() {
    beginCase("TC-07 (FR7) lane below threshold is not congested");
    const auto reports = parseReports(runSimulation(add(4, "W1") + add(4, "W2") + status()));

    check(reports.size() == 1, "status report is shown");
    if (reports.size() == 1) {
        check(reports[0].lanes.at("West").vehicles == 2 &&
                  reports[0].lanes.at("West").level == "LOW",
              "2 vehicles: West is reported LOW (not congested)");
    }
    endCase("TC-07");
}

// ---------------------------------------------------------------------------
// TC-08 — FR8: statistics
// ---------------------------------------------------------------------------
void tc08() {
    beginCase("TC-08 (FR8) traffic statistics");
    // V1 and V2 join North at tick 0; V1 leaves at tick 1 (waited 1) and
    // V2 at tick 2 (waited 2), so the average wait is 1.5 ticks.
    const std::string log =
        runSimulation(add(1, "V1") + add(1, "V2") + add(3, "E1") + advance(2) + status());
    const auto reports = parseReports(log);

    check(reports.size() == 1, "status report is shown");
    if (reports.size() == 1) {
        const StatusReport& r = reports[0];
        check(r.added == 3, "vehicles added = 3 (matches add commands)");
        check(r.processed == 2 && countOf(log, " moved from the ") == 2,
              "vehicles processed = 2 (matches move events)");
        check(contains(log, "V1 (car) moved from the North lane after waiting 1 tick(s)") &&
                  contains(log, "V2 (car) moved from the North lane after waiting 2 tick(s)"),
              "per-vehicle waiting times are 1 and 2 ticks");
        check(r.averageWait == 1.5, "average waiting time = 1.5 ticks");
        check(r.lanes.at("East").vehicles == 1 && r.lanes.at("North").vehicles == 0,
              "queue lengths are consistent with added minus processed");
    }
    endCase("TC-08");
}

// ---------------------------------------------------------------------------
// TC-09 — NFR1: representative workload
// (tests/test_performance.cpp runs a larger 20,000 / 200,000 vehicle version)
// ---------------------------------------------------------------------------
void tc09() {
    beginCase("TC-09 (NFR1) representative workload");
    const std::size_t vehicleCount = 5000;
    std::string script;
    for (std::size_t i = 0; i < vehicleCount; ++i) {
        script += add(static_cast<int>(i % 4) + 1, "P" + std::to_string(i));
    }
    const std::size_t ticks = (vehicleCount / 12 + 1) * 16 + 64;
    for (std::size_t t = 0; t < ticks; ++t) {
        script += advance();
        if (t % 500 == 0) {
            script += status();
        }
    }
    script += status();

    const auto start = std::chrono::steady_clock::now();
    const std::string log = runSimulation(script);
    const double ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - start).count();
    const auto reports = parseReports(log);

    check(contains(log, "Simulation ended."), "every command completed; program exited normally");
    check(!reports.empty() && reports.back().added == vehicleCount &&
              reports.back().processed == vehicleCount,
          "all 5,000 vehicles added and processed");
    check(!reports.empty() && totalQueued(reports.back()) == 0, "all lanes empty at the end");
    std::cout << "        " << vehicleCount << " vehicles, " << ticks << " ticks in " << ms
              << " ms\n";
    endCase("TC-09");
}

// ---------------------------------------------------------------------------
// TC-10 — NFR3, SR2: empty-queue guard and integrity
// ---------------------------------------------------------------------------
void tc10() {
    beginCase("TC-10 (NFR3, SR2) empty-queue operation and integrity");
    // Tick 1: North is GREEN with an empty queue (attempted dequeue on empty).
    // Then add A, B, C to North and let ticks 2 and 3 move A and B.
    const std::string log = runSimulation(status() + advance() + status() + add(1, "A") +
                                          add(1, "B") + add(1, "C") + status() + advance() +
                                          status() + advance() + status());
    const auto reports = parseReports(log);

    check(reports.size() == 5, "five status reports are shown");
    if (reports.size() == 5) {
        check(contains(log, "Tick 1: North lane has a green signal but no vehicles to process."),
              "move on an empty queue is guarded with a message");
        check(totalQueued(reports[1]) == 0 && reports[1].processed == 0,
              "guarded operation changed nothing");
        check(reports[2].lanes.at("North").queue == "A (car) -> B (car) -> C (car)",
              "after adding: A -> B -> C");
        check(reports[3].lanes.at("North").queue == "B (car) -> C (car)",
              "after one move: B -> C");
        check(reports[4].lanes.at("North").queue == "C (car)", "after two moves: C");
        check(reports[4].added == 3 && reports[4].processed == 2 &&
                  totalQueued(reports[4]) == 1,
              "none lost or duplicated (3 added = 2 processed + 1 queued)");
    }
    endCase("TC-10");
}

// ---------------------------------------------------------------------------
// TC-11 — SR1, SO2: invalid input is rejected
// ---------------------------------------------------------------------------
void tc11() {
    beginCase("TC-11 (SR1, SO2) invalid input is rejected");
    const std::string script =
        add(1, "OK1") +                          // one valid vehicle
        addRaw("0") +                            // lane below range
        addRaw("5") +                            // lane above range
        addRaw("abc") + "1.5\n" + "-1\n" +       // not a number, then not an integer, then negative
        "1\n2\n\n" +                             // empty vehicle ID
        "1\n2\n   \n" +                          // blank vehicle ID (spaces only)
        "1\n2\nKA09\n\n" +                       // empty vehicle type
        "1\n2\nKA10\n   \n" +                    // blank vehicle type (spaces only)
        "1\n3\nOK1\n" +                         // duplicate vehicle ID (rejected before type)
        status();

    const std::string log = runSimulation(script);
    const auto reports = parseReports(log);

    check(countOf(log, "Invalid lane. Vehicle was not added.") == 3,
          "lane 0, lane 5 and lane -1 are rejected with an error");
    check(countOf(log, "Enter a valid number.") == 2,
          "non-numeric lane input ('abc', '1.5') is rejected");
    check(countOf(log, "Vehicle ID cannot be empty.") == 2,
          "empty and blank (spaces only) vehicle IDs are rejected");
    check(countOf(log, "Vehicle type cannot be empty.") == 2,
          "empty and blank (spaces only) vehicle types are rejected");
    check(contains(log, "That vehicle ID has already been used."),
          "duplicate vehicle ID is rejected");
    check(countOf(log, " added to the ") == 1, "only the one valid vehicle was added");

    if (reports.size() == 1) {
        const StatusReport& r = reports[0];
        check(totalQueued(r) == 1 && r.lanes.at("North").queue == "OK1 (car)" &&
                  r.added == 1,
              "no queue was modified by invalid input");
    } else {
        check(false, "status report is shown");
    }
    endCase("TC-11");
}

// ---------------------------------------------------------------------------
// TC-12 — SO1: consistency across mixed valid and invalid operations
// ---------------------------------------------------------------------------
void tc12() {
    beginCase("TC-12 (SO1) mixed valid and invalid operations");
    const std::string mixed =
        add(1, "N1") + addRaw("9") + add(2, "S1") + "1\n2\n\n" +  // valid, bad lane, valid, empty ID
        advance() + "1\n1\nN1\n" + "7\n" +                      // tick 1, duplicate ID, bad menu option
        add(1, "N2") + "1\nxyz\n9\n" + "1\n3\nE1\n  \n" +          // valid, non-numeric then bad lane, blank type
        advance(4) + add(3, "E1") + advance(2);                   // ticks 2-5, valid, ticks 6-7

    const std::string validOnly =
        add(1, "N1") + add(2, "S1") + advance() + add(1, "N2") + advance(4) + add(3, "E1") +
        advance(2);

    const auto mixedReports = parseReports(runSimulation(mixed + status()));
    const auto validReports = parseReports(runSimulation(validOnly + status()));

    check(mixedReports.size() == 1 && validReports.size() == 1, "both runs show a final report");
    if (mixedReports.size() == 1 && validReports.size() == 1) {
        const StatusReport& m = mixedReports[0];
        const StatusReport& v = validReports[0];

        bool lanesMatch = true;
        for (const std::string& name : laneNames) {
            const LaneStatus& a = m.lanes.at(name);
            const LaneStatus& b = v.lanes.at(name);
            lanesMatch = lanesMatch && a.signal == b.signal && a.vehicles == b.vehicles &&
                         a.queue == b.queue && a.level == b.level;
        }

        check(m.tick == v.tick, "tick count matches the valid-only run");
        check(lanesMatch, "every lane's signal, queue and count match the valid-only run");
        check(m.added == v.added && m.processed == v.processed &&
                  m.averageWait == v.averageWait,
              "statistics match the valid-only run");
        check(m.added == 4 && m.processed + totalQueued(m) == 4,
              "4 valid vehicles: none lost or duplicated");
    }
    endCase("TC-12");
}

} // namespace

int main() {
    std::cout << "Traffic Management System - Test Plan execution (TC-01 to TC-12)\n";

    tc01();
    tc02();
    tc03();
    tc04();
    tc05();
    tc06();
    tc07();
    tc08();
    tc09();
    tc10();
    tc11();
    tc12();

    std::cout << "\n==================== SUMMARY ====================\n";
    int failed = 0;
    for (const auto& result : summary) {
        std::cout << "  " << result.first << "  " << (result.second ? "PASS" : "FAIL") << '\n';
        if (!result.second) {
            ++failed;
        }
    }
    std::cout << "=================================================\n"
              << summary.size() - failed << " of " << summary.size() << " test cases passed.\n";

    return failed == 0 ? 0 : 1;
}
