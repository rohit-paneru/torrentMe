#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <atomic>

// Cross-platform socket handling
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    typedef SOCKET socket_t;
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
    #define SOCKET_IN_PROGRESS WSAEINPROGRESS
    #define SOCKET_INVALID INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int socket_t;
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET(s) close(s)
    #define SOCKET_WOULD_BLOCK EWOULDBLOCK
    #define SOCKET_IN_PROGRESS EINPROGRESS
    #define SOCKET_INVALID -1
#endif

// Include configuration
#include "config.h"

// Constants
#define CHUNK_SIZE 1024
#define MAX_BUFFER_SIZE 4096

// Error codes
enum ErrorCode {
    SUCCESS = 0,
    ERROR_SOCKET_CREATE = 1,
    ERROR_SOCKET_CONNECT = 2,
    ERROR_SOCKET_BIND = 3,
    ERROR_SOCKET_LISTEN = 4,
    ERROR_SOCKET_ACCEPT = 5,
    ERROR_FILE_OPEN = 6,
    ERROR_FILE_READ = 7,
    ERROR_FILE_WRITE = 8,
    ERROR_TRACKER_CONNECT = 9,
    ERROR_PEER_CONNECT = 10,
    ERROR_INVALID_INPUT = 11,
    ERROR_CHECKSUM_MISMATCH = 12
};

// Utility functions
std::string getErrorMessage(ErrorCode code);
std::string calculateMD5(const std::string& filepath);
void initializeSockets();
void cleanupSockets();
bool isSocketValid(socket_t socket);

// Logger class for better logging
class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    static void setLogLevel(LogLevel level);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

private:
    static LogLevel currentLevel;
    static std::mutex logMutex;
    static void log(LogLevel level, const std::string& message);
};

// Progress bar for downloads
class ProgressBar {
public:
    ProgressBar(size_t total, size_t width = 50);
    void update(size_t current);
    void finish();

private:
    size_t total;
    size_t width;
    size_t lastProgress;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};

#endif // COMMON_H