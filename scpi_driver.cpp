#include <cstdint>
#include <iostream>

// Mock variables to simulate hardware registers
uint8_t mock_switch_reg = 0;
uint32_t mock_odometer_reg = 42; // Example simulated value

// Function to "write" to AXI register (simulation)
int write_to_axi(uint32_t addr, uint8_t value) {
    // Simulate register write
    std::cout << "[SIM] AXI write: Address = 0x" << std::hex << addr << ", Value = " << std::dec << static_cast<int>(value) << std::endl;
    mock_switch_reg = value; // Store value in mock variable
    return 0; // Success
}

// Function to "read" odometer register (simulation)
uint32_t read_odometer(uint32_t addr) {
    std::cout << "[SIM] AXI read: Address = 0x" << std::hex << addr << " -> Value = " << std::dec << mock_odometer_reg << std::endl;
    return mock_odometer_reg;
}