#include "common.h"


std::mutex Logger::logMutex;
Logger::LogLevel Logger::currentLevel = Logger::INFO;

// Error message mapping
std::string getErrorMessage(ErrorCode code) {
    static const std::map<ErrorCode, std::string> errorMessages = {
        {SUCCESS, "Operation completed successfully"},
        {ERROR_SOCKET_CREATE, "Failed to create socket"},
        {ERROR_SOCKET_CONNECT, "Failed to connect to remote host"},
        {ERROR_SOCKET_BIND, "Failed to bind socket to address"},
        {ERROR_SOCKET_LISTEN, "Failed to listen on socket"},
        {ERROR_SOCKET_ACCEPT, "Failed to accept connection"},
        {ERROR_FILE_OPEN, "Failed to open file"},
        {ERROR_FILE_READ, "Failed to read from file"},
        {ERROR_FILE_WRITE, "Failed to write to file"},
        {ERROR_TRACKER_CONNECT, "Failed to connect to tracker"},
        {ERROR_PEER_CONNECT, "Failed to connect to peer"},
        {ERROR_INVALID_INPUT, "Invalid user input"},
        {ERROR_CHECKSUM_MISMATCH, "File checksum verification failed"}
    };

    auto it = errorMessages.find(code);
    if (it != errorMessages.end()) {
        return it->second;
    }
    return "Unknown error";
}

// Socket initialization for Windows
void initializeSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::fatal("Failed to initialize Winsock");
        exit(1);
    }
#endif
}

// Socket cleanup for Windows
void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Check if socket is valid
bool isSocketValid(socket_t socket) {
#ifdef _WIN32
    return socket != INVALID_SOCKET;
#else
    return socket >= 0;
#endif
}

// Simple MD5 calculation (placeholder - in a real implementation, use a proper crypto library)
std::string calculateMD5(const std::string& filepath) {
    // This is a simplified placeholder. In a real implementation,
    // you would use a proper crypto library to calculate MD5.
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return "";
    }

    // Simple checksum calculation (not actual MD5)
    uint32_t checksum = 0;
    char buffer[CHUNK_SIZE];
    while (file.read(buffer, CHUNK_SIZE)) {
        for (int i = 0; i < file.gcount(); i++) {
            checksum = (checksum + static_cast<uint8_t>(buffer[i])) % UINT32_MAX;
        }
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << checksum;
    return ss.str();
}

// Logger implementation
void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string levelStr;
    switch (level) {
        case DEBUG:   levelStr = "DEBUG"; break;
        case INFO:    levelStr = "INFO"; break;
        case WARNING: levelStr = "WARNING"; break;
        case ERROR:   levelStr = "ERROR"; break;
        case FATAL:   levelStr = "FATAL"; break;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << levelStr << "] " << message << std::endl;
}

void Logger::debug(const std::string& message) {
    log(DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(INFO, message);
}

void Logger::warning(const std::string& message) {
    log(WARNING, message);
}

void Logger::error(const std::string& message) {
    log(ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(FATAL, message);
}

// Progress bar implementation
ProgressBar::ProgressBar(size_t total, size_t width)
    : total(total), width(width), lastProgress(0) {
    startTime = std::chrono::steady_clock::now();
}

void ProgressBar::update(size_t current) {
    if (total == 0) return;
    
    float progress = static_cast<float>(current) / total;
    int barWidth = static_cast<int>(progress * width);
    
    // Only update if progress has changed significantly
    if (static_cast<int>(progress * 100) > lastProgress) {
        lastProgress = static_cast<int>(progress * 100);
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        std::cout << "\r[";
        for (int i = 0; i < width; ++i) {
            if (i < barWidth) std::cout << "=";
            else if (i == barWidth) std::cout << ">";
            else std::cout << " ";
        }
        
        std::cout << "] " << static_cast<int>(progress * 100) << "% ";
        
        if (elapsed > 0 && progress > 0) {
            float eta = elapsed / progress - elapsed;
            std::cout << "ETA: " << static_cast<int>(eta) << "s";
        }
        
        std::cout << std::flush;
    }
}

void ProgressBar::finish() {
    update(total);
    std::cout << std::endl;
}