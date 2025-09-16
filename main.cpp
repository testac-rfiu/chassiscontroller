// Top of the file: fix Windows version detection for httplib
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0A00  // Windows 10/11

#ifdef WINVER
#undef WINVER
#endif
#define WINVER 0x0A00

#include "httplib.h"
#include "json.hpp"   // nlohmann/json
#include <iostream>
#include <string>

using namespace httplib;
using json = nlohmann::json;

// Hardcoded SCPI paths
json config = {
    {"paths", {
        { {"id", 1}, {"component_id", "switch1"}, {"address", 268436224}, {"gpio_value", 1} },
        { {"id", 2}, {"component_id", "switch1"}, {"address", 268436224}, {"gpio_value", 2} },
        { {"id", 3}, {"component_id", "switch1"}, {"address", 268436224}, {"gpio_value", 3} },
        { {"id", 4}, {"component_id", "switch2"}, {"address", 268436480}, {"gpio_value", 4} }
    }}
};

// Very basic SCPI command handler
std::string sendCommand(const std::string &cmd) {
    // Handle ":ROUTE:PATH n" or ":ROUTE:PATH? n"
    if (cmd.rfind(":ROUTE:PATH", 0) == 0) {
        try {
            // Check if it’s a query
            bool is_query = (cmd.find('?') != std::string::npos);

            // Extract number part after ":ROUTE:PATH"
            std::string num_str = cmd.substr(11);
            // Remove '?' and trim spaces
            num_str.erase(remove(num_str.begin(), num_str.end(), '?'), num_str.end());
            num_str.erase(0, num_str.find_first_not_of(" \t"));
            num_str.erase(num_str.find_last_not_of(" \t") + 1);

            int requested_id = std::stoi(num_str);

            // Look for a matching path in config
            for (auto &path : config["paths"]) {
                if (path["id"] == requested_id) {
                    // ✅ Always return the whole JSON object
                    return path.dump();
                }
            }
            return R"({"error":"Path ID not found"})";
        } catch (...) {
            return R"({"error":"Invalid SCPI command format"})";
        }
    }

    return R"({"error":"Unknown SCPI command"})";
}


int main() {
    Server svr;

    // Endpoint: handle SCPI commands
    svr.Post("/scpi", [](const Request &req, Response &res) {
        std::string command = req.body;
        std::cout << "📩 Received SCPI command: " << command << std::endl;

        std::string reply = sendCommand(command);
        res.set_content(reply, "text/plain");
    });

    // Endpoint: return all config paths
    svr.Get("/config", [](const Request &req, Response &res) {
        res.set_content(config.dump(4), "application/json");
    });

    // Endpoint: return a single path by ID
    svr.Get(R"(/config/(\d+))", [](const Request &req, Response &res) {
        int requested_id = std::stoi(req.matches[1]);

        for (auto &path : config["paths"]) {
            if (path["id"] == requested_id) {
                res.set_content(path.dump(4), "application/json");
                return;
            }
        }

        res.status = 404;
        res.set_content("{\"error\": \"Path not found\"}", "application/json");
    });

    std::cout << "🚀 Server listening on http://localhost:8080 ..." << std::endl;
    svr.listen("localhost", 8080);
}
