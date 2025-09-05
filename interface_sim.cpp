#include <iostream>
#include <string>
#include <nlohmann/json.hpp>  // Assuming nlohmann/json is installed (via vcpkg install nlohmann-json)

// Needs to be compiled with C++11, use: g++ interface_sim.cpp -I./include -std=c++11 -o interface_sim to compile,.\interface_sim.exe to run

// Mock hardware function to simulate AXI write
int write_to_axi(uint32_t addr, uint8_t value) {
    std::cout << "Mock AXI write: Address = " << addr << ", Value = " << static_cast<int>(value) << std::endl;
    return 0;  // Success
}

// Mock function to simulate reading odometer
uint32_t read_odometer() {
    std::cout << "Mock odometer read: Returning simulated value 42" << std::endl;
    return 42;  // Simulated value
}

// Function to parse and process SCPI command with JSON validation
void process_scpi_command(const std::string& scpi_cmd, const std::string& component_id, const nlohmann::json& config_json) {
    // Validate SCPI command (basic check: must start with "PATH:SELECT ")
    if (scpi_cmd.rfind("PATH:SELECT ", 0) != 0) {
        std::cout << "Invalid command format. Must be 'PATH:SELECT <number>'" << std::endl;
        return;
    }

    // Extract the path number (e.g., "3" from "PATH:SELECT 3")
    std::string path_str = scpi_cmd.substr(12); // Skip "PATH:SELECT "
    int id_num;
    try {
        id_num = std::stoi(path_str);
        if (id_num < 0 || id_num > 255) { // Assume 8-bit paths (0-255)
            std::cout << "Invalid path number. Must be 0-255" << std::endl;
            return;
        }
    } catch (...) {
        std::cout << "Invalid path number format" << std::endl;
        return;
    }

    // Validate against JSON configuration (assume JSON has a "paths" array with id, component_id, address, gpio_value)
    if (!config_json.contains("paths") || !config_json["paths"].is_array()) {
        std::cout << "JSON does not contain valid 'paths' array" << std::endl;
        return;
    }

    bool path_found = false;
    for (const auto& path : config_json["paths"]) {
        if (path["id"].get<int>() == id_num && path["component_id"].get<std::string>() == component_id) {
            path_found = true;
            // Extract address and gpio_value from the JSON
            uint32_t addr = path["address"].get<uint32_t>(); // Assume JSON has "address" field
            uint8_t gpio_value = path["gpio_value"].get<uint8_t>(); // Assume JSON has "gpio_value" field
            // Call mock hardware function
            if (write_to_axi(addr, gpio_value) == 0) {
                uint32_t odometer = read_odometer(); // Call mock read
                std::cout << "Success: Path " << id_num << " (" << component_id << ") activated. Odometer: " << odometer << std::endl;
            } else {
                std::cout << "Mock hardware write failed" << std::endl;
            }
            break;
        }
    }

    if (!path_found) {
        std::cout << "Path " << id_num << " with component_id " << component_id << " not found in JSON configuration" << std::endl;
    }
}

int main() {
    // Simulate receiving a SCPI command (hardcoded for standalone execution)
    std::string scpi_cmd = "PATH:SELECT 3";

    // Simulate component_id as a separate argument
    std::string component_id = "switch1";

    // Simulate a JSON configuration (hardcoded; in real scenario, load from file)
    std::string json_str = R"({
        "paths": [
            {"id": 1, "component_id": "switch1", "address": 268436224, "gpio_value": 1},
            {"id": 2, "component_id": "switch1", "address": 268436224, "gpio_value": 2},
            {"id": 3, "component_id": "switch1", "address": 268436224, "gpio_value": 3},
            {"id": 1, "component_id": "switch2", "address": 268436480, "gpio_value": 1}
        ]
    })";

    nlohmann::json config_json = nlohmann::json::parse(json_str);

    // Process the command
    process_scpi_command(scpi_cmd, component_id, config_json);

    return 0;
}