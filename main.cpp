
#include <array>
#include <cstddef>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>

enum class SignalState {
    Red,
    Yellow,
    Green
};

struct Vehicle {
    std::string id;
    std::string type;
    std::size_t arrivalTick;
};

struct Lane {
    std::string name;
    std::queue<Vehicle> vehicles;
    SignalState signalState = SignalState::Red;
};

class TrafficSimulation {
public:
    TrafficSimulation()
        : lanes_{{
              {"North", std::queue<Vehicle>{}, SignalState::Green},
              {"South", std::queue<Vehicle>{}, SignalState::Red},
              {"East", std::queue<Vehicle>{}, SignalState::Red},
              {"West", std::queue<Vehicle>{}, SignalState::Red}
          }} {}

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
    std::unordered_set<std::string> vehicleIds_;
    std::size_t greenLane_ = 0;
    std::size_t tick_ = 0;
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

        Lane& lane = lanes_[static_cast<std::size_t>(laneNumber - 1)];

        lane.vehicles.push({id, type, tick_});
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
                      << " - " << lane.vehicles.size() << " vehicle(s)";

            if (!lane.vehicles.empty()) {
                std::cout << ": ";

                std::queue<Vehicle> waiting = lane.vehicles;
                bool first = true;

                while (!waiting.empty()) {
                    if (!first) {
                        std::cout << ", ";
                    }

                    std::cout << waiting.front().id
                              << " (" << waiting.front().type << ')';

                    waiting.pop();
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

        if (lane.vehicles.empty()) {
            std::cout << "Tick " << tick_ << ": " << lane.name
                      << " lane has a green signal but no vehicles to process.\n";
        } else {
            const Vehicle vehicle = lane.vehicles.front();
            lane.vehicles.pop();

            const std::size_t waitingTime = tick_ - vehicle.arrivalTick;

            ++vehiclesProcessed_;
            totalWaitingTime_ += waitingTime;

            std::cout << "Tick " << tick_ << ": vehicle " << vehicle.id
                      << " (" << vehicle.type << ") moved from the "
                      << lane.name << " lane after waiting "
                      << waitingTime << " tick(s).\n";
        }

        // Transition the active lane from Green to Yellow.
        lane.signalState = SignalState::Yellow;
        std::cout << lane.name << " lane signal changed to YELLOW.\n";

        // Complete the transition and activate the next lane.
        lane.signalState = SignalState::Red;

        std::cout << lane.name << " lane signal changed to RED.\n";

        greenLane_ = (greenLane_ + 1) % lanes_.size();
        lanes_[greenLane_].signalState = SignalState::Green;

        std::cout << "Signal changed to " << lanes_[greenLane_].name
                  << " lane (GREEN).\n";
    }
};

int main() {
    TrafficSimulation simulation;
    simulation.run();
    return 0;
}
