#include <httplib.h>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

// SCPI driver functions (mocked)
int write_to_axi(uint32_t addr, uint8_t value) {
    std::cout << "SCPI Driver: Mock AXI write to address " << addr << " with value " << static_cast<int>(value) << std::endl;
    return 0;
}

uint32_t read_odometer() {
    std::cout << "SCPI Driver: Mock odometer read: 42" << std::endl;
    return 42;
}

nlohmann::json load_json_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open " << filename << std::endl;
        return nlohmann::json();
    }
    nlohmann::json j;
    file >> j;
    return j;
}

void process_scpi_command(const std::string& scpi_cmd) {
    std::cout << "Processing SCPI command: " << scpi_cmd << std::endl;
    nlohmann::json config_json = load_json_from_file("paths.json");
    if (config_json.empty()) {
        std::cout << "Failed to load paths.json" << std::endl;
        return;
    }

    if (scpi_cmd.rfind("PATH:SELECT ", 0) != 0) {
        std::cout << "Invalid command format" << std::endl;
        return;
    }

    std::string path_str = scpi_cmd.substr(12);
    int path_num;
    try {
        path_num = std::stoi(path_str);
        if (path_num < 0 || path_num > 255) {
            std::cout << "Invalid path number" << std::endl;
            return;
        }
    } catch (...) {
        std::cout << "Invalid path number format" << std::endl;
        return;
    }

    bool path_found = false;
    if (config_json.contains("paths") && config_json["paths"].is_array()) {
        for (const auto& path : config_json["paths"]) {
            if (path["id"].get<int>() == path_num) {
                path_found = true;
                uint32_t addr = path["address"].get<uint32_t>();
                uint8_t gpio_value = path["gpio_value"].get<uint8_t>();
                if (write_to_axi(addr, gpio_value) == 0) {
                    uint32_t odometer = read_odometer();
                    std::cout << "Path " << path_num << " activated at address " << addr
                              << " with GPIO value " << static_cast<int>(gpio_value)
                              << ". Odometer: " << odometer << std::endl;
                }
            }
        }
    }

    if (!path_found) {
        std::cout << "No paths found with id " << path_num << std::endl;
    }
}

int main() {
    std::cout << "Starting server..." << std::endl;
    httplib::Server server;

    std::cout << "Setting up endpoints..." << std::endl;
    server.Options("/command", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); // Allow all origins for testing
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200; // OK for preflight
    });

    server.Post("/command", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received request from " << req.remote_addr << ":" << req.remote_port << std::endl;
        std::cout << "Request body: " << req.body << std::endl;
        res.set_header("Access-Control-Allow-Origin", "*"); // Allow all origins for testing
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            if (!body.contains("scpi_command")) {
                res.set_content("Invalid request: Missing 'scpi_command' field", "text/plain");
                res.status = 400;
                return;
            }
            std::string scpi_cmd = body["scpi_command"];
            process_scpi_command(scpi_cmd);
            nlohmann::json response;
            response["status"] = "OK";
            response["message"] = "Commands processed from paths.json";
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            res.set_content("Invalid JSON or error: " + std::string(e.what()), "text/plain");
            res.status = 400;
        }
    });

    std::cout << "Attempting to bind to 0.0.0.0:8080..." << std::endl;
    try {
        server.listen("0.0.0.0", 8080);
    } catch (const std::exception& e) {
        std::cout << "Listen failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Server running (should not reach here)" << std::endl;
    return 0;
}