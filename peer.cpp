#include "common.h"
#include "network.h"
#include "fileutils.h"
#include "peer.h"
#include <csignal>
#include <limits>
#include <iomanip>

// Global peer instance for signal handling
Peer* g_peer = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down...");
    if (g_peer) {
        g_peer->stopSeeding();
    }
    exit(0);
}

Peer::Peer() : serverSocket(SOCKET_INVALID), seeding(false), seedingPort(0) {
}

Peer::~Peer() {
    stopSeeding();
}

bool Peer::seedFile(const std::string& filepath, int port) {
    // Check if file exists
    if (!FileUtils::fileExists(filepath)) {
        Logger::error("File does not exist: " + filepath);
        return false;
    }
    
    // Get filename from path
    std::string filename = FileUtils::getFilename(filepath);
    
    // Register with tracker
    if (!registerWithTracker(filename, port)) {
        Logger::error("Failed to register with tracker");
        return false;
    }
    
    // Start seeder thread
    startSeederThread(filepath, port);
    
    return true;
}

void Peer::startSeederThread(const std::string& filepath, int port) {
    // Stop any existing seeder thread
    stopSeeding();
    
    // Set current seeding file and port
    currentSeedingFile = filepath;
    seedingPort = port;
    
    // Create socket
    ErrorCode error;
    serverSocket = Network::createSocket(&error);
    if (!isSocketValid(serverSocket)) {
        Logger::error("Failed to create seeder socket: " + getErrorMessage(error));
        return;
    }
    
    // Bind socket
    if (!Network::bindSocket(serverSocket, port, &error)) {
        Logger::error("Failed to bind seeder socket: " + getErrorMessage(error));
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
        return;
    }
    
    // Listen for connections
    if (!Network::listenOnSocket(serverSocket, 5, &error)) {
        Logger::error("Failed to listen on seeder socket: " + getErrorMessage(error));
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
        return;
    }
    
    seeding = true;
    
    // Start seeder thread
    seederThread = std::thread([this, filepath]() {
        Logger::info("Seeder started on port " + std::to_string(seedingPort));
        
        while (seeding) {
            std::string clientIp;
            int clientPort;
            ErrorCode error;
            
            socket_t clientSock = Network::acceptConnection(serverSocket, &clientIp, &clientPort, &error);
            if (!isSocketValid(clientSock)) {
                if (seeding) {
                    Logger::error("Failed to accept connection: " + getErrorMessage(error));
                }
                continue;
            }
            
            Logger::info("Accepted download request from " + clientIp + ":" + std::to_string(clientPort));
            
            // Handle download request in a new thread
            std::thread([this, clientSock, filepath]() {
                // Calculate file size and checksum
                size_t fileSize = FileUtils::getFileSize(filepath);
                std::string checksum = FileUtils::calculateChecksum(filepath);
                
                // Send file size
                if (!Network::sendData(clientSock, &fileSize, sizeof(fileSize))) {
                    Logger::error("Failed to send file size");
                    Network::closeSocket(clientSock);
                    return;
                }
                
                // Send checksum
                size_t checksumSize = checksum.size();
                if (!Network::sendData(clientSock, &checksumSize, sizeof(checksumSize))) {
                    Logger::error("Failed to send checksum size");
                    Network::closeSocket(clientSock);
                    return;
                }
                
                if (!Network::sendData(clientSock, checksum.c_str(), checksumSize)) {
                    Logger::error("Failed to send checksum");
                    Network::closeSocket(clientSock);
                    return;
                }
                
                // Open file
                std::ifstream file(filepath, std::ios::binary);
                if (!file.is_open()) {
                    Logger::error("Failed to open file for sending: " + filepath);
                    Network::closeSocket(clientSock);
                    return;
                }
                
                // Send file data
                char buffer[CHUNK_SIZE];
                size_t totalSent = 0;
                size_t bytesRead;
                
                while (FileUtils::readFileChunk(file, buffer, CHUNK_SIZE, bytesRead)) {
                    if (!Network::sendData(clientSock, buffer, bytesRead)) {
                        Logger::error("Failed to send file chunk");
                        file.close();
                        Network::closeSocket(clientSock);
                        return;
                    }
                    
                    totalSent += bytesRead;
                    
                    // Log progress
                    if (totalSent % (CHUNK_SIZE * 100) == 0) {
                        double progress = static_cast<double>(totalSent) / fileSize * 100.0;
                        Logger::info("Upload progress: " + std::to_string(static_cast<int>(progress)) + "%");
                    }
                }
                
                Logger::info("File sent successfully: " + filepath);
                file.close();
                Network::closeSocket(clientSock);
            }).detach();
        }
    });
}

void Peer::stopSeeding() {
    if (!seeding) {
        return;
    }
    
    seeding = false;
    
    // Close server socket
    if (isSocketValid(serverSocket)) {
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
    }
    
    // Wait for seeder thread to finish
    if (seederThread.joinable()) {
        seederThread.join();
    }
    
    Logger::info("Stopped seeding");
}

bool Peer::registerWithTracker(const std::string& filename, int port) {
    ErrorCode error;
    
    // Create socket
    socket_t sock = Network::createSocket(&error);
    if (!isSocketValid(sock)) {
        Logger::error("Failed to create socket: " + getErrorMessage(error));
        return false;
    }
    
    // Connect to tracker
    if (!Network::connectToHost(sock, TRACKER_IP, TRACKER_PORT, &error)) {
        Logger::error("Failed to connect to tracker: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    // Send REGISTER command
    std::string command = "REGISTER " + filename + " " + std::to_string(port) + "\n";
    if (!Network::sendData(sock, command.c_str(), command.size(), &error)) {
        Logger::error("Failed to send REGISTER command: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    // Receive response
    char buffer[MAX_BUFFER_SIZE] = {0};
    if (Network::receiveData(sock, buffer, MAX_BUFFER_SIZE - 1, &error) <= 0) {
        Logger::error("Failed to receive response: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    // Close socket
    Network::closeSocket(sock);
    
    // Check response
    std::string response(buffer);
    if (response != "OK\n") {
        Logger::error("Tracker returned error: " + response);
        return false;
    }
    
    Logger::info("Registered file '" + filename + "' with tracker");
    return true;
}

std::vector<std::string> Peer::getPeersFromTracker(const std::string& filename) {
    ErrorCode error;
    
    // Create socket
    socket_t sock = Network::createSocket(&error);
    if (!isSocketValid(sock)) {
        Logger::error("Failed to create socket: " + getErrorMessage(error));
        return {};
    }
    
    // Connect to tracker
    if (!Network::connectToHost(sock, TRACKER_IP, TRACKER_PORT, &error)) {
        Logger::error("Failed to connect to tracker: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return {};
    }
    
    // Send GETPEERS command
    std::string command = "GETPEERS " + filename + "\n";
    if (!Network::sendData(sock, command.c_str(), command.size(), &error)) {
        Logger::error("Failed to send GETPEERS command: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return {};
    }
    
    // Receive response
    char buffer[MAX_BUFFER_SIZE] = {0};
    if (Network::receiveData(sock, buffer, MAX_BUFFER_SIZE - 1, &error) <= 0) {
        Logger::error("Failed to receive response: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return {};
    }
    
    // Close socket
    Network::closeSocket(sock);
    
    // Parse response
    std::string response(buffer);
    std::vector<std::string> peers;
    
    if (response == "\n") {
        return peers;
    }
    
    std::string peerList = response.substr(0, response.find('\n'));
    std::istringstream iss(peerList);
    std::string peer;
    
    while (std::getline(iss, peer, ';')) {
        if (!peer.empty()) {
            peers.push_back(peer);
        }
    }
    
    return peers;
}

bool Peer::downloadFile(const std::string& filename, const std::string& destPath) {
    // Get peers from tracker
    auto peers = getPeersFromTracker(filename);
    if (peers.empty()) {
        Logger::error("No peers found for file: " + filename);
        return false;
    }
    
    Logger::info("Found " + std::to_string(peers.size()) + " peers for file: " + filename);
    
    // Display peers
    std::cout << "Available peers:\n";
    for (size_t i = 0; i < peers.size(); ++i) {
        std::cout << "  [" << i << "] " << peers[i] << "\n";
    }
    
    // Select peer
    int index = -1;
    std::string input;
    std::cout << "\nSelect peer index: ";
    std::getline(std::cin, input);
    
    try {
        index = std::stoi(input);
    } catch (...) {
        Logger::error("Invalid input. Please enter a number.");
        return false;
    }
    
    if (index < 0 || index >= peers.size()) {
        Logger::error("Invalid index selected.");
        return false;
    }
    
    // Parse peer address
    std::string peerAddr = peers[index];
    size_t colonPos = peerAddr.find(':');
    if (colonPos == std::string::npos) {
        Logger::error("Invalid peer address: " + peerAddr);
        return false;
    }
    
    std::string peerIp = peerAddr.substr(0, colonPos);
    int peerPort;
    
    try {
        peerPort = std::stoi(peerAddr.substr(colonPos + 1));
    } catch (...) {
        Logger::error("Invalid peer port: " + peerAddr.substr(colonPos + 1));
        return false;
    }
    
    // Connect to peer
    ErrorCode error;
    socket_t sock = Network::createSocket(&error);
    if (!isSocketValid(sock)) {
        Logger::error("Failed to create socket: " + getErrorMessage(error));
        return false;
    }
    
    Logger::info("Connecting to peer " + peerIp + ":" + std::to_string(peerPort));
    
    if (!Network::connectToHost(sock, peerIp, peerPort, &error)) {
        Logger::error("Failed to connect to peer: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    // Receive file size
    size_t fileSize;
    if (Network::receiveData(sock, &fileSize, sizeof(fileSize), &error) != sizeof(fileSize)) {
        Logger::error("Failed to receive file size: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    // Receive checksum
    size_t checksumSize;
    if (Network::receiveData(sock, &checksumSize, sizeof(checksumSize), &error) != sizeof(checksumSize)) {
        Logger::error("Failed to receive checksum size: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    char checksumBuffer[256];
    if (checksumSize >= sizeof(checksumBuffer)) {
        Logger::error("Checksum size too large");
        Network::closeSocket(sock);
        return false;
    }
    
    if (Network::receiveData(sock, checksumBuffer, checksumSize, &error) != checksumSize) {
        Logger::error("Failed to receive checksum: " + getErrorMessage(error));
        Network::closeSocket(sock);
        return false;
    }
    
    checksumBuffer[checksumSize] = '\0';
    std::string expectedChecksum(checksumBuffer, checksumSize);
    
    // Create progress bar
    ProgressBar progress(fileSize);
    
    // Open destination file
    std::ofstream file(destPath, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open destination file: " + destPath);
        Network::closeSocket(sock);
        return false;
    }
    
    // Receive file data
    char buffer[CHUNK_SIZE];
    size_t totalReceived = 0;
    ssize_t bytesReceived;
    
    while (totalReceived < fileSize) {
        size_t bytesToReceive = std::min(static_cast<size_t>(CHUNK_SIZE), fileSize - totalReceived);
        bytesReceived = Network::receiveData(sock, buffer, bytesToReceive, &error);
        
        if (bytesReceived <= 0) {
            Logger::error("Failed to receive file data: " + getErrorMessage(error));
            file.close();
            Network::closeSocket(sock);
            return false;
        }
        
        if (!FileUtils::writeFileChunk(file, buffer, bytesReceived)) {
            Logger::error("Failed to write file chunk");
            file.close();
            Network::closeSocket(sock);
            return false;
        }
        
        totalReceived += bytesReceived;
        progress.update(totalReceived);
    }
    
    progress.finish();
    file.close();
    Network::closeSocket(sock);
    
    // Verify file integrity
    if (!FileUtils::verifyFileIntegrity(destPath, expectedChecksum)) {
        Logger::error("File integrity check failed");
        return false;
    }
    
    Logger::info("Download complete: " + destPath);
    return true;
}

// Main function with improved UI
int main() {
    // Initialize network subsystem
    Network::initialize();
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create peer
    Peer peer;
    g_peer = &peer;
    
    // Main menu loop
    bool running = true;
    while (running) {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════╗\n";
        std::cout << "║       MINI-TORRENT PEER          ║\n";
        std::cout << "╠══════════════════════════════════╣\n";
        std::cout << "║ 1. Seed a file                   ║\n";
        std::cout << "║ 2. Download a file               ║\n";
        std::cout << "║ 3. Exit                          ║\n";
        std::cout << "╚══════════════════════════════════╝\n";
        std::cout << "Select option: ";
        
        std::string input;
        std::getline(std::cin, input);
        
        int option;
        try {
            option = std::stoi(input);
        } catch (...) {
            std::cout << "❌ Invalid input. Please enter a number.\n";
            continue;
        }
        
        switch (option) {
            case 1: { // Seed a file
                std::string filePath;
                std::cout << "Enter path to file to seed: ";
                std::getline(std::cin, filePath);
                
                // Strip quotes from file path
                if (!filePath.empty() && filePath.front() == '"')
                    filePath.erase(0, 1);
                if (!filePath.empty() && filePath.back() == '"')
                    filePath.pop_back();
                
                int port;
                std::cout << "Enter port to serve on (>=1024): ";
                std::string portStr;
                std::getline(std::cin, portStr);
                
                try {
                    port = std::stoi(portStr);
                } catch (...) {
                    std::cout << "❌ Invalid port number.\n";
                    break;
                }
                
                if (port < 1024) {
                    std::cout << "❌ Port must be >= 1024.\n";
                    break;
                }
                
                if (peer.seedFile(filePath, port)) {
                    std::cout << "✅ File is now being seeded. Press Ctrl+C to stop.\n";
                    
                    // Wait for user to press Enter to return to menu
                    std::cout << "Press Enter to return to menu (seeding will continue in background)...\n";
                    std::cin.get();
                } else {
                    std::cout << "❌ Failed to seed file.\n";
                }
                break;
            }
            
            case 2: { // Download a file
                std::string filename;
                std::cout << "Enter filename to download: ";
                std::getline(std::cin, filename);
                
                std::string destPath;
                std::cout << "Enter destination path: ";
                std::getline(std::cin, destPath);
                
                // Strip quotes from destination path
                if (!destPath.empty() && destPath.front() == '"')
                    destPath.erase(0, 1);
                if (!destPath.empty() && destPath.back() == '"')
                    destPath.pop_back();
                
                if (peer.downloadFile(filename, destPath)) {
                    std::cout << "✅ File downloaded successfully.\n";
                } else {
                    std::cout << "❌ Failed to download file.\n";
                }
                break;
            }
            
            case 3: // Exit
                std::cout << "Goodbye!\n";
                running = false;
                break;
                
            default:
                std::cout << "❌ Invalid option. Please try again.\n";
                break;
        }
    }
    
    // Stop seeding
    peer.stopSeeding();
    
    // Cleanup
    Network::cleanup();
    
    return 0;
}
