#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "common.h"

// File utility class
class FileUtils {
public:
    // Check if a file exists
    static bool fileExists(const std::string& filepath);
    
    // Get file size
    static size_t getFileSize(const std::string& filepath);
    
    // Calculate file checksum
    static std::string calculateChecksum(const std::string& filepath);
    
    // Extract filename from path
    static std::string getFilename(const std::string& filepath);
    
    // Read file in chunks
    static bool readFileChunk(std::ifstream& file, char* buffer, size_t chunkSize, size_t& bytesRead);
    
    // Write file chunk
    static bool writeFileChunk(std::ofstream& file, const char* buffer, size_t bytesToWrite);
    
    // Send file over socket
    static bool sendFile(socket_t sock, const std::string& filepath, ProgressBar* progress = nullptr);
    
    // Receive file from socket
    static bool receiveFile(socket_t sock, const std::string& filepath, size_t fileSize, ProgressBar* progress = nullptr);
    
    // Verify file integrity
    static bool verifyFileIntegrity(const std::string& filepath, const std::string& expectedChecksum);
};

#endif // FILEUTILS_H