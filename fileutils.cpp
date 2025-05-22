#include "fileutils.h"
#include "network.h"

bool FileUtils::fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

size_t FileUtils::getFileSize(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }
    
    return static_cast<size_t>(file.tellg());
}

std::string FileUtils::calculateChecksum(const std::string& filepath) {
    return calculateMD5(filepath);
}

std::string FileUtils::getFilename(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    if (pos == std::string::npos) {
        return filepath;
    }
    
    return filepath.substr(pos + 1);
}

bool FileUtils::readFileChunk(std::ifstream& file, char* buffer, size_t chunkSize, size_t& bytesRead) {
    if (!file.is_open()) {
        Logger::error("File not open for reading");
        return false;
    }
    
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();
    
    return bytesRead > 0;
}

bool FileUtils::writeFileChunk(std::ofstream& file, const char* buffer, size_t bytesToWrite) {
    if (!file.is_open()) {
        Logger::error("File not open for writing");
        return false;
    }
    
    file.write(buffer, bytesToWrite);
    return !file.fail();
}

bool FileUtils::sendFile(socket_t sock, const std::string& filepath, ProgressBar* progress) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open file for sending: " + filepath);
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Send file size first
    if (!Network::sendData(sock, &fileSize, sizeof(fileSize))) {
        Logger::error("Failed to send file size");
        file.close();
        return false;
    }
    
    // Calculate and send checksum
    std::string checksum = calculateChecksum(filepath);
    size_t checksumSize = checksum.size();
    
    if (!Network::sendData(sock, &checksumSize, sizeof(checksumSize))) {
        Logger::error("Failed to send checksum size");
        file.close();
        return false;
    }
    
    if (!Network::sendData(sock, checksum.c_str(), checksumSize)) {
        Logger::error("Failed to send checksum");
        file.close();
        return false;
    }
    
    // Send file data
    char buffer[CHUNK_SIZE];
    size_t totalSent = 0;
    size_t bytesRead;
    
    while (readFileChunk(file, buffer, CHUNK_SIZE, bytesRead)) {
        if (!Network::sendData(sock, buffer, bytesRead)) {
            Logger::error("Failed to send file chunk");
            file.close();
            return false;
        }
        
        totalSent += bytesRead;
        
        if (progress) {
            progress->update(totalSent);
        }
    }
    
    if (progress) {
        progress->finish();
    }
    
    file.close();
    return true;
}

bool FileUtils::receiveFile(socket_t sock, const std::string& filepath, size_t fileSize, ProgressBar* progress) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open file for writing: " + filepath);
        return false;
    }
    
    // Receive file size
    size_t expectedFileSize;
    if (Network::receiveData(sock, &expectedFileSize, sizeof(expectedFileSize)) != sizeof(expectedFileSize)) {
        Logger::error("Failed to receive file size");
        file.close();
        return false;
    }
    
    // If fileSize is provided, use it, otherwise use the received size
    if (fileSize == 0) {
        fileSize = expectedFileSize;
    }
    
    // Receive checksum
    size_t checksumSize;
    if (Network::receiveData(sock, &checksumSize, sizeof(checksumSize)) != sizeof(checksumSize)) {
        Logger::error("Failed to receive checksum size");
        file.close();
        return false;
    }
    
    char checksumBuffer[256];
    if (checksumSize >= sizeof(checksumBuffer)) {
        Logger::error("Checksum size too large");
        file.close();
        return false;
    }
    
    if (Network::receiveData(sock, checksumBuffer, checksumSize) != checksumSize) {
        Logger::error("Failed to receive checksum");
        file.close();
        return false;
    }
    
    checksumBuffer[checksumSize] = '\0';
    std::string expectedChecksum(checksumBuffer, checksumSize);
    
    // Receive file data
    char buffer[CHUNK_SIZE];
    size_t totalReceived = 0;
    ssize_t bytesReceived;
    
    if (progress) {
        progress = new ProgressBar(fileSize);
    }
    
    while (totalReceived < fileSize) {
        size_t bytesToReceive = std::min(static_cast<size_t>(CHUNK_SIZE), fileSize - totalReceived);
        bytesReceived = Network::receiveData(sock, buffer, bytesToReceive);
        
        if (bytesReceived <= 0) {
            Logger::error("Failed to receive file chunk");
            file.close();
            if (progress) {
                delete progress;
            }
            return false;
        }
        
        if (!writeFileChunk(file, buffer, bytesReceived)) {
            Logger::error("Failed to write file chunk");
            file.close();
            if (progress) {
                delete progress;
            }
            return false;
        }
        
        totalReceived += bytesReceived;
        
        if (progress) {
            progress->update(totalReceived);
        }
    }
    
    if (progress) {
        progress->finish();
        delete progress;
    }
    
    file.close();
    
    // Verify file integrity
    return verifyFileIntegrity(filepath, expectedChecksum);
}

bool FileUtils::verifyFileIntegrity(const std::string& filepath, const std::string& expectedChecksum) {
    std::string actualChecksum = calculateChecksum(filepath);
    
    if (actualChecksum != expectedChecksum) {
        Logger::error("File integrity check failed: " + filepath);
        Logger::error("Expected checksum: " + expectedChecksum);
        Logger::error("Actual checksum: " + actualChecksum);
        return false;
    }
    
    Logger::info("File integrity verified: " + filepath);
    return true;
}