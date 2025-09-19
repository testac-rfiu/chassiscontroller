#include <httplib.h>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

// Global state to track current path
int current_path_id = -1; // -1 means no path selected

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
    // Note: SCPI command logging now happens in the endpoint before this function
    nlohmann::json config_json = load_json_from_file("paths.json");
    if (config_json.empty()) {
        std::cout << "Failed to load paths.json" << std::endl;
        return;
    }

    // Handle PATH:SELECT command
    if (scpi_cmd.rfind("PATH:SELECT ", 0) == 0) {
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

        // Update current path state
        current_path_id = path_num;
        std::cout << "Current path set to: " << current_path_id << std::endl;

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
            current_path_id = -1; // Reset if path not found
        }
        return;
    }

    // Handle SWITCH:SELECT command (with GPIO value parameter, respects current path for address)
    if (scpi_cmd.rfind("SWITCH:SELECT ", 0) == 0) {
        std::string params = scpi_cmd.substr(14); // Remove "SWITCH:SELECT "
        std::istringstream iss(params);
        std::string switch_str, gpio_str;
        
        if (!(iss >> switch_str >> gpio_str)) {
            std::cout << "Invalid SWITCH:SELECT format. Use: SWITCH:SELECT <switch_id> <gpio_value>" << std::endl;
            return;
        }

        std::string switch_id = switch_str;
        int gpio_value;
        try {
            gpio_value = std::stoi(gpio_str);
            if (gpio_value < 0 || gpio_value > 255) {
                std::cout << "Invalid GPIO value. Must be between 0 and 255" << std::endl;
                return;
            }
        } catch (...) {
            std::cout << "Invalid GPIO value format" << std::endl;
            return;
        }

        // Check if a path is currently selected
        if (current_path_id == -1) {
            std::cout << "No path currently selected. Please select a path first before using switch mode." << std::endl;
            return;
        }

        std::cout << "Switch ID received: " << switch_id << " with GPIO value: " << gpio_value << " (using path " << current_path_id << " address)" << std::endl;
        
        // Map SW1/SW2 to switch1/switch2 format for address lookup
        std::string component_id;
        if (switch_id == "SW1") {
            component_id = "switch1";
        } else if (switch_id == "SW2") {
            component_id = "switch2";
        } else {
            std::cout << "Unknown switch ID: " << switch_id << std::endl;
            return;
        }

        // Find the switch entry that matches the current path and component
        bool switch_found = false;
        if (config_json.contains("paths") && config_json["paths"].is_array()) {
            for (const auto& path : config_json["paths"]) {
                // Look for entry that matches both the current path ID and the component
                if (path["id"].get<int>() == current_path_id && 
                    path.contains("component_id") && 
                    path["component_id"].get<std::string>() == component_id) {
                    
                    switch_found = true;
                    uint32_t addr = path["address"].get<uint32_t>();
                    
                    if (write_to_axi(addr, static_cast<uint8_t>(gpio_value)) == 0) {
                        uint32_t odometer = read_odometer();
                        std::cout << "Switch " << switch_id << " (" << component_id << ") activated at address " << addr
                                  << " with custom GPIO value " << gpio_value 
                                  << " from path " << current_path_id
                                  << ". Odometer: " << odometer << std::endl;
                    }
                    break; // Found the exact match for this path
                }
            }
        }

        if (!switch_found) {
            std::cout << "Switch " << switch_id << " not found in path " << current_path_id << std::endl;
        }
        return;
    }

    // Handle SWITCH:INFO? command (query component information)
    if (scpi_cmd.rfind("SWITCH:INFO? ", 0) == 0) {
        std::string switch_num_str = scpi_cmd.substr(13);
        switch_num_str.erase(switch_num_str.find_last_not_of(" \t\r\n") + 1);
        
        // Map switch number to switch ID
        std::string switch_id = "SW" + switch_num_str;
        
        // Load components configuration
        nlohmann::json components_config = load_json_from_file("components_paths.json");
        
        if (!components_config.empty() && components_config.contains("components")) {
            // Find the requested switch component
            for (const auto& component : components_config["components"]) {
                if (component.contains("id") && component["id"].get<std::string>() == switch_id) {
                    // Store component info for the response (handled in the endpoint)
                    return;
                }
            }
        }
        return;
    }

    // If we get here, command format was not recognized
    std::cout << "Unknown command format. Supported commands:" << std::endl;
    std::cout << "  PATH:SELECT <path_id>" << std::endl;
    std::cout << "  SWITCH:SELECT <switch_id>" << std::endl;
    std::cout << "  SWITCH:INFO? <switch_number>" << std::endl;
}

int main() {
    std::cout << "Starting server..." << std::endl;
    httplib::Server server;

    std::cout << "Setting up endpoints..." << std::endl;
    
    // CORS preflight handler
    server.Options("/command", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    // Main command endpoint
    server.Post("/command", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received request from " << req.remote_addr << ":" << req.remote_port << std::endl;
        std::cout << "Request body: " << req.body << std::endl;
        
        res.set_header("Access-Control-Allow-Origin", "*");
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
            
            // Log all SCPI commands
            std::cout << "Processing SCPI command: " << scpi_cmd << std::endl;
            
            // Special handling for SWITCH:INFO? command
            if (scpi_cmd.rfind("SWITCH:INFO? ", 0) == 0) {
                std::string switch_num_str = scpi_cmd.substr(13);
                switch_num_str.erase(switch_num_str.find_last_not_of(" \t\r\n") + 1);
                
                std::string switch_id = "SW" + switch_num_str;
                
                // Load components configuration
                nlohmann::json components_config = load_json_from_file("components_paths.json");
                
                nlohmann::json response;
                response["status"] = "OK";
                
                if (!components_config.empty() && components_config.contains("components")) {
                    // Find the requested switch component
                    bool component_found = false;
                    for (const auto& component : components_config["components"]) {
                        if (component.contains("id") && component["id"].get<std::string>() == switch_id) {
                            response["component_info"] = component;
                            response["message"] = "Component information retrieved successfully";
                            component_found = true;
                            break;
                        }
                    }
                    
                    if (!component_found) {
                        response["status"] = "ERROR";
                        response["message"] = "Component " + switch_id + " not found";
                    }
                } else {
                    response["status"] = "ERROR";
                    response["message"] = "Failed to load components configuration";
                }
                
                res.set_content(response.dump(), "application/json");
                res.status = 200;
                return;
            }
            
            // Process other SCPI commands normally
            process_scpi_command(scpi_cmd);
            
            nlohmann::json response;
            response["status"] = "OK";
            response["message"] = "Command processed successfully";
            response["current_path"] = current_path_id;
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            res.set_content("Invalid JSON or error: " + std::string(e.what()), "text/plain");
            res.status = 400;
        }
    });

    // Enhanced configuration endpoint - combines both files
    server.Get("/config", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        // Load both configuration files
        nlohmann::json paths_config = load_json_from_file("paths.json");
        nlohmann::json components_config = load_json_from_file("components_paths.json");
        
        nlohmann::json combined_config;
        
        // Add paths from paths.json
        if (!paths_config.empty() && paths_config.contains("paths")) {
            combined_config["paths"] = paths_config["paths"];
        } else {
            combined_config["paths"] = nlohmann::json::array();
        }
        
        // Add components from components_paths.json
        if (!components_config.empty() && components_config.contains("components")) {
            combined_config["components"] = components_config["components"];
        } else {
            combined_config["components"] = nlohmann::json::array();
        }
        
        // Add current path information
        combined_config["current_path"] = current_path_id;
        
        res.set_content(combined_config.dump(4), "application/json");
    });

    // Status endpoint to check current path
    server.Get("/status", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        nlohmann::json status;
        status["current_path"] = current_path_id;
        status["server_status"] = "running";
        
        // Add file status information
        std::ifstream paths_file("paths.json");
        std::ifstream components_file("components_paths.json");
        
        status["files"]["paths_json"] = paths_file.good() ? "found" : "missing";
        status["files"]["components_paths_json"] = components_file.good() ? "found" : "missing";
        
        paths_file.close();
        components_file.close();
        
        res.set_content(status.dump(4), "application/json");
    });

    std::cout << "Attempting to bind to localhost:8080..." << std::endl;
    
    if (!server.listen("localhost", 8080)) {
        std::cout << "❌ Failed to bind to localhost:8080. Port might be in use." << std::endl;
        std::cout << "Trying 0.0.0.0:8080..." << std::endl;
        
        if (!server.listen("0.0.0.0", 8080)) {
            std::cout << "❌ Failed to bind to 0.0.0.0:8080 as well." << std::endl;
            std::cout << "Please check if port 8080 is already in use." << std::endl;
            return 1;
        }
    }
    
    std::cout << "✅ Server is running successfully!" << std::endl;
    return 0;
}