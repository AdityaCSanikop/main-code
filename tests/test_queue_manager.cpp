// Unit tests for the Queue Manager (issue #2).
// Build and run from the repo root:
//   g++ -std=c++14 -Wall -Wextra tests/test_queue_manager.cpp -o test_queue_manager.out
//   ./test_queue_manager.out

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

Vehicle makeVehicle(const std::string& id) {
    return {id, "car", 0};
}

// TC-02 (FR2, FR5): vehicles stay in per-lane FIFO order.
void testFifoOrder() {
    std::cout << "TC-02 FIFO order and queue length\n";
    QueueManager queues(4);

    queues.enqueue(0, makeVehicle("V1"));
    queues.enqueue(0, makeVehicle("V2"));
    queues.enqueue(0, makeVehicle("V3"));

    check(queues.getQueueLength(0) == 3, "length is 3 after adding V1, V2, V3");
    Vehicle v;

    check(queues.peekFront(0, v) && v.id == "V1",
          "peekFront shows V1 without removing it");
    check(queues.getQueueLength(0) == 3, "length is still 3 after peekFront");

    check(queues.dequeue(0, v) && v.id == "V1", "V1 leaves first");
    check(queues.getQueueLength(0) == 2, "length becomes 2");

    check(queues.dequeue(0, v) && v.id == "V2", "V2 leaves second");
}

// FR2: each lane has its own queue.
void testLanesAreIndependent() {
    std::cout << "FR2 lanes are independent\n";
    QueueManager queues(4);

    queues.enqueue(0, makeVehicle("N1"));
    queues.enqueue(2, makeVehicle("E1"));
    queues.enqueue(2, makeVehicle("E2"));

    check(queues.getQueueLength(0) == 1, "lane 0 has 1 vehicle");
    check(queues.getQueueLength(1) == 0, "lane 1 has 0 vehicles");
    check(queues.getQueueLength(2) == 2, "lane 2 has 2 vehicles");

    Vehicle v;
    check(queues.dequeue(2, v) && v.id == "E1", "dequeue from lane 2 gives E1");
    check(queues.getQueueLength(0) == 1, "lane 0 is unaffected");
}

// TC-10 (NFR3, SR2): invalid operations are refused without corrupting state.
void testEmptyQueueGuard() {
    std::cout << "TC-10 empty-queue guard and integrity\n";
    QueueManager queues(4);

    Vehicle v;

    check(!queues.dequeue(1, v), "dequeue on an empty queue is refused");
    check(!queues.peekFront(1, v), "peekFront on an empty queue is refused");
    check(queues.getQueueLength(1) == 0, "empty queue still has length 0");

    queues.enqueue(1, makeVehicle("A"));
    queues.enqueue(1, makeVehicle("B"));
    queues.dequeue(1, v);
    queues.dequeue(1, v);
    check(!queues.dequeue(1, v), "dequeue after draining is refused");

    queues.enqueue(1, makeVehicle("C"));
    const std::vector<Vehicle> snapshot = queues.snapshot(1);
    check(snapshot.size() == 1 && snapshot[0].id == "C",
          "after refused operations only C is queued");
}

// SR1/SR2: invalid lane IDs are rejected and change nothing.
void testInvalidLane() {
    std::cout << "SR2 invalid lane IDs\n";
    QueueManager queues(4);

    check(!queues.enqueue(4, makeVehicle("X")), "enqueue to lane 4 is rejected");
    Vehicle v;
    check(!queues.dequeue(99, v), "dequeue from lane 99 is refused");
    check(queues.getQueueLength(4) == 0, "length of an invalid lane is 0");

    bool allEmpty = true;
    for (std::size_t lane = 0; lane < queues.laneCount(); ++lane) {
        allEmpty = allEmpty && queues.isEmpty(lane);
    }
    check(allEmpty, "no valid lane was modified");
}

// NFR3/SR2: the same vehicle cannot be queued twice, including after exit.
void testNoDuplicates() {
    std::cout << "NFR3 no duplicated vehicles\n";
    QueueManager queues(4);

    check(queues.enqueue(0, makeVehicle("D1")), "D1 is added");
    check(!queues.enqueue(0, makeVehicle("D1")), "D1 again in the same lane is rejected");
    check(!queues.enqueue(3, makeVehicle("D1")), "D1 again in another lane is rejected");
    check(queues.getQueueLength(0) == 1 && queues.getQueueLength(3) == 0,
          "only one copy of D1 exists");

    Vehicle out;
    check(queues.dequeue(0, out) && out.id == "D1", "D1 leaves the queue once");
    check(!queues.enqueue(3, makeVehicle("D1")),
          "a departed vehicle ID cannot be added again");
    check(queues.getQueueLength(0) == 0 && queues.getQueueLength(3) == 0,
          "rejecting a departed ID does not change any queue");
    check(!queues.enqueue(0, Vehicle{"", "car", 0}),
          "a vehicle with an empty ID is rejected");
}

} // namespace

int main() {
    testFifoOrder();
    testLanesAreIndependent();
    testEmptyQueueGuard();
    testInvalidLane();
    testNoDuplicates();

    std::cout << '\n'
              << (failures == 0 ? "All tests passed.\n" : "Some tests failed.\n");
    return failures == 0 ? 0 : 1;
}
