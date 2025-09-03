#include <crow.h>
#include <string>
#include <iostream>
#include "scpi_driver.h"  // Include the SCPI driver header

// Define AXI register address (example; adjust based on your FPGA memory map)
#define AXI_SWITCH_REG 0xA0001000  // Memory-mapped AXI register address

int main() {
    crow::SimpleApp app;

    // Route for handling path selection requests from the front end
    CROW_ROUTE(app, "/select_path") // POST request to /select_path
    .methods("POST"_method)
    ([](const crow::request& req) {
        // Extract the request body (assume JSON with "command" field)
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        // Extract the command from the JSON
        std::string command = body["command"].s();

        // Validate the command (basic check: must start with "PATH:SELECT ")
        if (command.rfind("PATH:SELECT ", 0) != 0) {
            return crow::response(400, "Invalid command format. Must be 'PATH:SELECT <number>'");
        }

        // Extract the path number (e.g., "3" from "PATH:SELECT 3")
        std::string path_str = command.substr(12); // Skip "PATH:SELECT "
        int path_num;
        try {
            path_num = std::stoi(path_str);
            if (path_num < 0 || path_num > 255) { // Assume 8-bit paths (0-255)
                return crow::response(400, "Invalid path number. Must be 0-255");
            }
        } catch (...) {
            return crow::response(400, "Invalid path number");
        }

        // Prepare command packet (e.g., 8-bit value for path)
        uint8_t command_packet = static_cast<uint8_t>(path_num);

        // Call the SCPI driver to write to AXI register
        if (write_to_axi(AXI_SWITCH_REG, command_packet) == 0) {
            // Success: Read back odometer or confirmation (example)
            uint32_t odometer = read_odometer();  // Assume a function in scpi_driver.h
            crow::json::wvalue response;
            response["status"] = "OK";
            response["path"] = path_num;
            response["odometer"] = odometer;
            return crow::response(200, response);
        } else {
            return crow::response(500, "Hardware write failed");
        }
    });

    // Run the server on port 8080
    app.port(8080).multithreaded().run();
}
