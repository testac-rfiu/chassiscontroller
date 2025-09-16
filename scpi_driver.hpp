#ifndef SCPI_DRIVER_HPP
#define SCPI_DRIVER_HPP

#include <string>

class SCPIDriver {
public:
    SCPIDriver();
    ~SCPIDriver();

    bool connect(const std::string& host, int port);
    std::string sendCommand(const std::string& cmd);

private:
    int sockfd;  // socket handle
    bool connected;
};

#endif
