#include "scpi_driver.hpp"
#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

SCPIDriver::SCPIDriver() : sockfd(-1), connected(false) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
}

SCPIDriver::~SCPIDriver() {
    if (connected) {
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
    }
}

bool SCPIDriver::connect(const std::string& host, int port) {
#ifdef _WIN32
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#else
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (sockfd < 0) return false;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host.c_str());

    if (::connect(sockfd, (sockaddr*)&server, sizeof(server)) < 0) {
        return false;
    }

    connected = true;
    return true;
}

std::string SCPIDriver::sendCommand(const std::string& cmd) {
    if (!connected) return "Not connected";

    send(sockfd, cmd.c_str(), cmd.size(), 0);

    char buffer[1024] = {0};
    int n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        return std::string(buffer);
    }
    return "";
}
