#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

// Network utility class
class Network {
public:
    // Initialize network subsystem
    static void initialize();
    
    // Clean up network subsystem
    static void cleanup();
    
    // Create a socket
    static socket_t createSocket(ErrorCode* error = nullptr);
    
    // Connect to a remote host
    static bool connectToHost(socket_t sock, const std::string& ip, int port, ErrorCode* error = nullptr);
    
    // Bind socket to a port
    static bool bindSocket(socket_t sock, int port, ErrorCode* error = nullptr);
    
    // Listen for connections
    static bool listenOnSocket(socket_t sock, int backlog = 5, ErrorCode* error = nullptr);
    
    // Accept a connection
    static socket_t acceptConnection(socket_t sock, std::string* clientIp = nullptr, int* clientPort = nullptr, ErrorCode* error = nullptr);
    
    // Send data
    static bool sendData(socket_t sock, const void* data, size_t length, ErrorCode* error = nullptr);
    
    // Receive data
    static ssize_t receiveData(socket_t sock, void* buffer, size_t length, ErrorCode* error = nullptr);
    
    // Close a socket
    static void closeSocket(socket_t sock);
    
    // Get last socket error
    static int getLastError();
    
    // Get error string
    static std::string getErrorString(int errorCode);
};

#endif // NETWORK_H