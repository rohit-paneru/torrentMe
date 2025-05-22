#include "network.h"

void Network::initialize() {
    initializeSockets();
}

void Network::cleanup() {
    cleanupSockets();
}

socket_t Network::createSocket(ErrorCode* error) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!isSocketValid(sock)) {
        if (error) *error = ERROR_SOCKET_CREATE;
        Logger::error("Failed to create socket: " + getErrorString(getLastError()));
        return SOCKET_INVALID;
    }
    return sock;
}

bool Network::connectToHost(socket_t sock, const std::string& ip, int port, ErrorCode* error) {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
#else
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
#endif
        if (error) *error = ERROR_SOCKET_CONNECT;
        Logger::error("Invalid IP address: " + ip);
        return false;
    }
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (error) *error = ERROR_SOCKET_CONNECT;
        Logger::error("Failed to connect to " + ip + ":" + std::to_string(port) + 
                     " - " + getErrorString(getLastError()));
        return false;
    }
    
    return true;
}

bool Network::bindSocket(socket_t sock, int port, ErrorCode* error) {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    // Set socket option to reuse address
    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (error) *error = ERROR_SOCKET_BIND;
        Logger::error("Failed to bind socket to port " + std::to_string(port) + 
                     " - " + getErrorString(getLastError()));
        return false;
    }
    
    return true;
}

bool Network::listenOnSocket(socket_t sock, int backlog, ErrorCode* error) {
    if (listen(sock, backlog) < 0) {
        if (error) *error = ERROR_SOCKET_LISTEN;
        Logger::error("Failed to listen on socket: " + getErrorString(getLastError()));
        return false;
    }
    
    return true;
}

socket_t Network::acceptConnection(socket_t sock, std::string* clientIp, int* clientPort, ErrorCode* error) {
    struct sockaddr_in addr{};
    socklen_t addrLen = sizeof(addr);
    
    socket_t clientSock = accept(sock, (struct sockaddr*)&addr, &addrLen);
    if (!isSocketValid(clientSock)) {
        if (error) *error = ERROR_SOCKET_ACCEPT;
        Logger::error("Failed to accept connection: " + getErrorString(getLastError()));
        return SOCKET_INVALID;
    }
    
    if (clientIp) {
        char ipStr[INET_ADDRSTRLEN];
#ifdef _WIN32
        *clientIp = inet_ntoa(addr.sin_addr);
#else
        inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
        *clientIp = ipStr;
#endif
    }
    
    if (clientPort) {
        *clientPort = ntohs(addr.sin_port);
    }
    
    return clientSock;
}

bool Network::sendData(socket_t sock, const void* data, size_t length, ErrorCode* error) {
    const char* buffer = static_cast<const char*>(data);
    size_t totalSent = 0;
    
    while (totalSent < length) {
        ssize_t sent = send(sock, buffer + totalSent, length - totalSent, 0);
        if (sent <= 0) {
            if (error) *error = ERROR_SOCKET_CONNECT;
            Logger::error("Failed to send data: " + getErrorString(getLastError()));
            return false;
        }
        
        totalSent += sent;
    }
    
    return true;
}

ssize_t Network::receiveData(socket_t sock, void* buffer, size_t length, ErrorCode* error) {
    ssize_t received = recv(sock, static_cast<char*>(buffer), length, 0);
    if (received < 0) {
        if (error) *error = ERROR_SOCKET_CONNECT;
        Logger::error("Failed to receive data: " + getErrorString(getLastError()));
    }
    
    return received;
}

void Network::closeSocket(socket_t sock) {
    if (isSocketValid(sock)) {
        CLOSE_SOCKET(sock);
    }
}

int Network::getLastError() {
    return SOCKET_ERROR_CODE;
}

std::string Network::getErrorString(int errorCode) {
#ifdef _WIN32
    char* errorMsg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errorMsg, 0, NULL);
    
    std::string result = errorMsg ? errorMsg : "Unknown error";
    LocalFree(errorMsg);
    return result;
#else
    return std::string(strerror(errorCode));
#endif
}