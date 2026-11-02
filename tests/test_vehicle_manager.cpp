// Unit tests for the Vehicle Manager (issue #5).
// Build and run from the repo root:
//   g++ -std=c++17 -Wall -Wextra -pedantic tests/test_vehicle_manager.cpp -o test_vehicle_manager.out
//   ./test_vehicle_manager.out

#include <iostream>
#include <string>

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

// FR5 entry rule: valid vehicles join the back of a lane in any signal state.
void testEntry() {
    std::cout << "FR5 entry rule\n";
    QueueManager queues(4);
    VehicleManager vehicles(queues);

    check(vehicles.addVehicle(0, "V1", "car", 0), "V1 added to lane 0");
    check(vehicles.addVehicle(0, "V2", "bus", 1), "V2 added to lane 0");
    check(queues.getQueueLength(0) == 2, "lane 0 has 2 vehicles");

    const std::vector<Vehicle> lane0 = queues.snapshot(0);
    check(lane0.size() == 2 && lane0[0].id == "V1" && lane0[1].id == "V2",
          "V2 joined behind V1");
    check(lane0[1].type == "bus" && lane0[1].arrivalTick == 1,
          "vehicle type and arrival tick are stored");
}

// FR5 entry rule: invalid vehicles never reach a queue.
void testRejectedEntry() {
    std::cout << "FR5 rejected entry\n";
    QueueManager queues(4);
    VehicleManager vehicles(queues);

    check(!vehicles.addVehicle(0, "", "car", 0), "empty vehicle ID is rejected");
    check(!vehicles.addVehicle(0, "V1", "", 0), "empty vehicle type is rejected");
    check(!vehicles.addVehicle(7, "V1", "car", 0), "invalid lane is rejected");

    vehicles.addVehicle(0, "V1", "car", 0);
    check(!vehicles.addVehicle(2, "V1", "car", 0), "queued vehicle ID cannot be added twice");

    std::size_t total = 0;
    for (std::size_t lane = 0; lane < queues.laneCount(); ++lane) {
        total += queues.getQueueLength(lane);
    }
    check(total == 1, "only the one valid vehicle is queued");
}

// TC-04 (FR5, FR3): a vehicle leaves only once its lane is GREEN.
void testExitOnlyOnGreen() {
    std::cout << "TC-04 exit only on GREEN\n";
    QueueManager queues(4);
    VehicleManager vehicles(queues);
    Vehicle out;

    vehicles.addVehicle(1, "V1", "car", 0);

    check(vehicles.moveVehicle(1, SignalState::Red, out) == MoveResult::BlockedBySignal,
          "move on RED is blocked");
    check(queues.getQueueLength(1) == 1, "V1 is still queued during RED");

    check(vehicles.moveVehicle(1, SignalState::Yellow, out) == MoveResult::BlockedBySignal,
          "move on YELLOW is blocked");
    check(queues.getQueueLength(1) == 1, "V1 is still queued during YELLOW");

    check(vehicles.moveVehicle(1, SignalState::Green, out) == MoveResult::Moved,
          "move on GREEN succeeds");
    check(out.id == "V1", "V1 is the vehicle that left");
    check(queues.getQueueLength(1) == 0, "lane is empty after V1 leaves");

    check(VehicleManager::canLeave(SignalState::Green) &&
              !VehicleManager::canLeave(SignalState::Yellow) &&
              !VehicleManager::canLeave(SignalState::Red),
          "canLeave is true only for GREEN");
}

// TC-02 (FR2, FR5): vehicles leave through the Vehicle Manager in FIFO order.
void testExitFifoOrder() {
    std::cout << "TC-02 exit in FIFO order\n";
    QueueManager queues(4);
    VehicleManager vehicles(queues);
    Vehicle out;

    vehicles.addVehicle(2, "V1", "car", 0);
    vehicles.addVehicle(2, "V2", "car", 0);
    vehicles.addVehicle(2, "V3", "car", 0);
    check(queues.getQueueLength(2) == 3, "length is 3 before any move");

    vehicles.moveVehicle(2, SignalState::Green, out);
    check(out.id == "V1", "V1 leaves first");
    check(queues.getQueueLength(2) == 2, "length becomes 2");

    vehicles.moveVehicle(2, SignalState::Green, out);
    check(out.id == "V2", "V2 leaves second");
}

// NFR3/SR2: moving from an empty or invalid lane changes nothing.
void testExitFromEmptyLane() {
    std::cout << "NFR3 exit from empty or invalid lane\n";
    QueueManager queues(4);
    VehicleManager vehicles(queues);
    Vehicle out;

    check(vehicles.moveVehicle(3, SignalState::Green, out) == MoveResult::QueueEmpty,
          "GREEN on an empty lane reports QueueEmpty");
    check(vehicles.moveVehicle(9, SignalState::Green, out) == MoveResult::QueueEmpty,
          "invalid lane is refused");

    vehicles.addVehicle(0, "V1", "car", 0);
    check(vehicles.moveVehicle(3, SignalState::Green, out) == MoveResult::QueueEmpty,
          "GREEN on lane 3 does not take vehicles from lane 0");
    check(queues.getQueueLength(0) == 1, "lane 0 is unaffected");
}

} // namespace

int main() {
    testEntry();
    testRejectedEntry();
    testExitOnlyOnGreen();
    testExitFifoOrder();
    testExitFromEmptyLane();

    std::cout << '\n'
              << (failures == 0 ? "All tests passed.\n" : "Some tests failed.\n");
    return failures == 0 ? 0 : 1;
}
