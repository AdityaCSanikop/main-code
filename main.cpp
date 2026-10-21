
#include <array>
#include <cstddef>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

// A simulated vehicle (SRS §1.3).
struct Vehicle {
    std::string id;
    std::string type;
    std::size_t arrivalTick;
};

// Queue Manager — Data Structure Layer (SADS §2, §4).
//
// Owns one FIFO queue per lane and is the only component allowed to change
// queue contents. Every operation that could corrupt queue state is refused
// instead of attempted (SADS §10):
//   - unknown lane ID          -> rejected, nothing changes
//   - dequeue on empty queue   -> returns false, nothing changes
//   - vehicle ID already queued -> rejected, so a vehicle is never duplicated
//
// Requirements: FR2 (maintain queues), FR5 (entry/exit), FR7 (queue length
// for congestion), NFR1 (O(1) enqueue/dequeue/length), NFR3 and SR2
// (no loss, duplication or corruption of vehicles).
class QueueManager {
public:
    explicit QueueManager(std::size_t laneCount) : queues_(laneCount) {}

    std::size_t laneCount() const { return queues_.size(); }

    bool isValidLane(std::size_t laneId) const {
        return laneId < queues_.size();
    }

    // Adds the vehicle to the back of the lane's queue.
    // Returns false (and changes nothing) for an invalid lane or a vehicle
    // ID that is already waiting in any queue.
    bool enqueue(std::size_t laneId, const Vehicle& vehicle) {
        if (!isValidLane(laneId) || queuedIds_.count(vehicle.id) != 0) {
            return false;
        }

        queues_[laneId].push_back(vehicle);
        queuedIds_.insert(vehicle.id);
        return true;
    }

    // Removes the front vehicle of the lane's queue and copies it into
    // `out`. Returns false (and changes nothing) for an invalid lane or an
    // empty queue.
    bool dequeue(std::size_t laneId, Vehicle& out) {
        if (!isValidLane(laneId) || queues_[laneId].empty()) {
            return false;
        }

        out = queues_[laneId].front();
        queues_[laneId].pop_front();
        queuedIds_.erase(out.id);
        return true;
    }

    // Copies the front vehicle into `out` without removing it.
    // Returns false for an invalid lane or an empty queue.
    bool peekFront(std::size_t laneId, Vehicle& out) const {
        if (!isValidLane(laneId) || queues_[laneId].empty()) {
            return false;
        }

        out = queues_[laneId].front();
        return true;
    }

    // Number of vehicles waiting in the lane (0 for an invalid lane).
    std::size_t getQueueLength(std::size_t laneId) const {
        return isValidLane(laneId) ? queues_[laneId].size() : 0;
    }

    bool isEmpty(std::size_t laneId) const {
        return getQueueLength(laneId) == 0;
    }

    // Read-only, front-to-back copy of a lane's queue for display.
    // The internal storage itself is never exposed (SR2).
    std::vector<Vehicle> snapshot(std::size_t laneId) const {
        if (!isValidLane(laneId)) {
            return {};
        }

        return {queues_[laneId].begin(), queues_[laneId].end()};
    }

private:
    std::vector<std::deque<Vehicle>> queues_;
    std::unordered_set<std::string> queuedIds_;
};

enum class SignalState {
    Red,
    Yellow,
    Green
};

struct Lane {
    std::string name;
    SignalState signalState = SignalState::Red;
};

class TrafficSimulation {
public:
    TrafficSimulation()
        : lanes_{{
              {"North", SignalState::Green},
              {"South", SignalState::Red},
              {"East", SignalState::Red},
              {"West", SignalState::Red}
          }},
          queues_(lanes_.size()) {}

    void run() {
        std::cout << "Traffic Management System (Simulation)\n";

        bool running = true;

        while (running) {
            printMenu();
            const int choice = readInteger("Choose an option: ");

            switch (choice) {
            case 1:
                addVehicle();
                break;
            case 2:
                showStatus();
                break;
            case 3:
                advanceSimulation();
                break;
            case 4:
                running = false;
                break;
            default:
                std::cout << "Please choose an option from 1 to 4.\n";
            }
        }

        std::cout << "Simulation ended.\n";
    }

private:
    static const char* signalStateName(SignalState state) {
        switch (state) {
        case SignalState::Red:
            return "RED";
        case SignalState::Yellow:
            return "YELLOW";
        case SignalState::Green:
            return "GREEN";
        }

        return "UNKNOWN";
    }

    std::array<Lane, 4> lanes_;
    QueueManager queues_;
    std::unordered_set<std::string> vehicleIds_;
    std::size_t greenLane_ = 0;
    std::size_t tick_ = 0;
    std::size_t greenTicks_ = 0;
    static constexpr std::size_t GREEN_DURATION_TICKS = 3;
    
    std::size_t vehiclesAdded_ = 0;
    std::size_t vehiclesProcessed_ = 0;
    std::size_t totalWaitingTime_ = 0;

    static void printMenu() {
        std::cout << "\n1. Add vehicle\n"
                  << "2. View traffic status\n"
                  << "3. Advance simulation\n"
                  << "4. Exit\n";
    }

    static int readInteger(const std::string& prompt) {
        while (true) {
            std::cout << prompt;

            std::string input;

            if (!std::getline(std::cin, input)) {
                return 4;
            }

            std::istringstream stream(input);
            int value = 0;
            char extra = '\0';

            if (stream >> value && !(stream >> extra)) {
                return value;
            }

            std::cout << "Enter a valid number.\n";
        }
    }

    void addVehicle() {
        std::cout << "\nSelect a lane:\n";

        for (std::size_t i = 0; i < lanes_.size(); ++i) {
            std::cout << i + 1 << ". " << lanes_[i].name << '\n';
        }

        const int laneNumber = readInteger("Lane number: ");

        if (laneNumber < 1 ||
            static_cast<std::size_t>(laneNumber) > lanes_.size()) {
            std::cout << "Invalid lane. Vehicle was not added.\n";
            return;
        }

        std::string id;
        std::cout << "Vehicle ID: ";

        if (!std::getline(std::cin, id)) {
            return;
        }

        if (id.empty()) {
            std::cout << "Vehicle ID cannot be empty. Vehicle was not added.\n";
            return;
        }

        if (vehicleIds_.count(id) != 0) {
            std::cout << "That vehicle ID has already been used. "
                      << "Vehicle was not added.\n";
            return;
        }

        std::string type;
        std::cout << "Vehicle type: ";

        if (!std::getline(std::cin, type)) {
            return;
        }

        if (type.empty()) {
            std::cout << "Vehicle type cannot be empty. Vehicle was not added.\n";
            return;
        }

        const std::size_t laneId = static_cast<std::size_t>(laneNumber - 1);
        const Lane& lane = lanes_[laneId];

        if (!queues_.enqueue(laneId, {id, type, tick_})) {
            std::cout << "Vehicle could not be queued. Vehicle was not added.\n";
            return;
        }

        vehicleIds_.insert(id);
        ++vehiclesAdded_;

        std::cout << "Vehicle " << id << " added to the "
                  << lane.name << " lane queue.\n";
    }

    void showStatus() const {
        std::cout << "\nTraffic status at tick " << tick_ << ":\n";

        for (std::size_t i = 0; i < lanes_.size(); ++i) {
            const Lane& lane = lanes_[i];

            std::cout << lane.name << " lane"
                      << " [" << signalStateName(lane.signalState) << "]"
                      << " - " << queues_.getQueueLength(i) << " vehicle(s)";

            if (!queues_.isEmpty(i)) {
                std::cout << ": ";

                bool first = true;

                for (const Vehicle& vehicle : queues_.snapshot(i)) {
                    if (!first) {
                        std::cout << ", ";
                    }

                    std::cout << vehicle.id << " (" << vehicle.type << ')';
                    first = false;
                }
            }

            std::cout << '\n';
        }

        const double averageWaitingTime =
            vehiclesProcessed_ == 0
                ? 0.0
                : static_cast<double>(totalWaitingTime_) /
                      vehiclesProcessed_;

        std::cout << "Vehicles added: " << vehiclesAdded_
                  << " | Vehicles processed: " << vehiclesProcessed_
                  << " | Average waiting time: " << averageWaitingTime
                  << " tick(s)\n";
    }

void advanceSimulation() {
    ++tick_;

    Lane& lane = lanes_[greenLane_];

    // If the current lane is Yellow, finish its transition.
    if (lane.signalState == SignalState::Yellow) {
        lane.signalState = SignalState::Red;

        std::cout << lane.name
                  << " lane signal changed to RED.\n";

        // Activate the next lane.
        greenLane_ = (greenLane_ + 1) % lanes_.size();

        lanes_[greenLane_].signalState = SignalState::Green;
        greenTicks_ = 0;

        std::cout << "Signal changed to "
                  << lanes_[greenLane_].name
                  << " lane (GREEN).\n";

        return;
    }

    // Process a vehicle only when the current lane is Green.
    Vehicle vehicle;

    if (!queues_.dequeue(greenLane_, vehicle)) {
        std::cout << "Tick " << tick_ << ": "
                  << lane.name
                  << " lane has a green signal but no vehicles to process.\n";
    } else {
        const std::size_t waitingTime =
            tick_ - vehicle.arrivalTick;

        ++vehiclesProcessed_;
        totalWaitingTime_ += waitingTime;

        std::cout << "Tick " << tick_
                  << ": vehicle " << vehicle.id
                  << " (" << vehicle.type
                  << ") moved from the "
                  << lane.name
                  << " lane after waiting "
                  << waitingTime << " tick(s).\n";
    }

    // Count how long this lane has been Green.
    ++greenTicks_;

    // Change to Yellow after the Green duration is complete.
    if (greenTicks_ >= GREEN_DURATION_TICKS) {
        lane.signalState = SignalState::Yellow;

        std::cout << lane.name
                  << " lane signal changed to YELLOW.\n";
    }
}
};

#ifndef TMS_UNIT_TEST
int main() {
    TrafficSimulation simulation;
    simulation.run();
    return 0;
}
#endif // TMS_UNIT_TEST
