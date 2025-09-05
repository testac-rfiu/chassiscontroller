#include <cstdint>
#include <iostream>
#include <string>

// Mock hardware function to simulate AXI write
int write_to_axi(uint32_t addr, uint8_t value) {
    std::cout << "Mock AXI write: Address = " << addr << ", Value = " << static_cast<int>(value) << std::endl;
    return 0;  // Success
}

// Function to process SCPI command
void process_scpi_command(const std::string& scpi_cmd) {
    // Validate SCPI command (basic check: must start with "PATH:SELECT ")
    if (scpi_cmd.rfind("PATH:SELECT ", 0) != 0) {
        std::cout << "Invalid command format. Must be 'PATH:SELECT <number>'" << std::endl;
        return;
    }

    // Extract the path number (e.g., "3" from "PATH:SELECT 3")
    std::string path_str = scpi_cmd.substr(12); // Skip "PATH:SELECT "
    int path_num;
    try {
        path_num = std::stoi(path_str);
        if (path_num < 0 || path_num > 255) { // Assume 8-bit paths (0-255)
            std::cout << "Invalid path number. Must be 0-255" << std::endl;
            return;
        }
    } catch (...) {
        std::cout << "Invalid path number format" << std::endl;
        return;
    }

    // Prepare command packet (e.g., 8-bit value for path)
    uint8_t command_packet = static_cast<uint8_t>(path_num);

    // Call mock hardware function
    if (write_to_axi(0xA0001000, command_packet) == 0) { // Simulated address
        std::cout << "Success: Path " << path_num << " activated." << std::endl;
    } else {
        std::cout << "Mock hardware write failed" << std::endl;
    }
}

int main() {
    // Simulate receiving a SCPI command (hardcoded for standalone execution)
    std::string scpi_cmd = "PATH:SELECT 3";

    // Process the command
    process_scpi_command(scpi_cmd);

    return 0;
}