// tracker.cpp
#include "common.h"
#include "network.h"
#include "tracker.h"
#include <csignal>

// Global tracker instance for signal handling
Tracker* g_tracker = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    Logger::info("Received signal " + std::to_string(signal) + ", shutting down...");
    if (g_tracker) {
        g_tracker->stop();
    }
    exit(0);
}

Tracker::Tracker(int port) : port(port), serverSocket(SOCKET_INVALID), running(false) {
}

Tracker::~Tracker() {
    stop();
}

bool Tracker::start() {
    ErrorCode error;
    
    // Create socket
    serverSocket = Network::createSocket(&error);
    if (!isSocketValid(serverSocket)) {
        Logger::error("Failed to create tracker socket: " + getErrorMessage(error));
        return false;
    }
    
    // Bind socket
    if (!Network::bindSocket(serverSocket, port, &error)) {
        Logger::error("Failed to bind tracker socket: " + getErrorMessage(error));
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
        return false;
    }
    
    // Listen for connections
    if (!Network::listenOnSocket(serverSocket, 10, &error)) {
        Logger::error("Failed to listen on tracker socket: " + getErrorMessage(error));
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
        return false;
    }
    
    running = true;
    Logger::info("Tracker started on port " + std::to_string(port));
    
    // Accept connections in a loop
    std::thread([this]() {
        while (running) {
            std::string clientIp;
            int clientPort;
            ErrorCode error;
            
            socket_t clientSock = Network::acceptConnection(serverSocket, &clientIp, &clientPort, &error);
            if (!isSocketValid(clientSock)) {
                if (running) {
                    Logger::error("Failed to accept connection: " + getErrorMessage(error));
                }
                continue;
            }
            
            Logger::info("Accepted connection from " + clientIp + ":" + std::to_string(clientPort));
            
            // Handle client in a new thread
            clientThreads.push_back(std::thread(&Tracker::handleClient, this, clientSock, clientIp, clientPort));
        }
    }).detach();
    
    return true;
}

void Tracker::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // Close server socket
    if (isSocketValid(serverSocket)) {
        Network::closeSocket(serverSocket);
        serverSocket = SOCKET_INVALID;
    }
    
    // Wait for all client threads to finish
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    clientThreads.clear();
    
    Logger::info("Tracker stopped");
}

bool Tracker::registerFile(const std::string& filename, const std::string& peerIp, int peerPort) {
    std::string peerId = peerIp + ":" + std::to_string(peerPort);
    
    std::lock_guard<std::mutex> lock(filePeersMutex);
    auto& peers = filePeers[filename];
    
    // Check if peer already registered
    if (std::find(peers.begin(), peers.end(), peerId) != peers.end()) {
        return true;
    }
    
    peers.push_back(peerId);
    Logger::info("Registered file '" + filename + "' with peer " + peerId);
    
    return true;
}

std::vector<std::string> Tracker::getPeers(const std::string& filename) {
    std::lock_guard<std::mutex> lock(filePeersMutex);
    
    auto it = filePeers.find(filename);
    if (it == filePeers.end()) {
        return {};
    }
    
    return it->second;
}

void Tracker::handleClient(socket_t clientSock, const std::string& clientIp, int clientPort) {
    char buffer[MAX_BUFFER_SIZE] = {0};
    ErrorCode error;
    
    // Receive request
    ssize_t bytesReceived = Network::receiveData(clientSock, buffer, MAX_BUFFER_SIZE - 1, &error);
    if (bytesReceived <= 0) {
        Logger::error("Failed to receive data from client: " + getErrorMessage(error));
        Network::closeSocket(clientSock);
        return;
    }
    
    buffer[bytesReceived] = '\0';
    std::string request(buffer);
    std::string response;
    
    // Parse request and generate response
    parseRequest(request, clientIp, clientPort, response);
    
    // Send response
    if (!Network::sendData(clientSock, response.c_str(), response.size(), &error)) {
        Logger::error("Failed to send response to client: " + getErrorMessage(error));
    }
    
    // Close connection
    Network::closeSocket(clientSock);
}

void Tracker::parseRequest(const std::string& request, const std::string& clientIp, int clientPort, std::string& response) {
    std::istringstream iss(request);
    std::string command;
    iss >> command;
    
    if (command == "REGISTER") {
        // REGISTER <filename> <peer_port>
        std::string filename;
        int peerPort;
        
        iss >> filename >> peerPort;
        
        if (filename.empty() || peerPort <= 0) {
            response = "ERROR Invalid REGISTER command format\n";
            return;
        }
        
        if (registerFile(filename, clientIp, peerPort)) {
            response = "OK\n";
        } else {
            response = "ERROR Failed to register file\n";
        }
    }
    else if (command == "GETPEERS") {
        // GETPEERS <filename>
        std::string filename;
        iss >> filename;
        
        if (filename.empty()) {
            response = "ERROR Invalid GETPEERS command format\n";
            return;
        }
        
        auto peers = getPeers(filename);
        if (peers.empty()) {
            response = "\n";
        } else {
            for (const auto& peer : peers) {
                response += peer + ";";
            }
            response += "\n";
        }
    }
    else {
        response = "ERROR Unknown command\n";
    }
}

int main() {
    // Initialize network subsystem
    Network::initialize();
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create and start tracker
    Tracker tracker(TRACKER_PORT);
    g_tracker = &tracker;
    
    if (!tracker.start()) {
        Logger::fatal("Failed to start tracker");
        Network::cleanup();
        return 1;
    }
    
    Logger::info("Tracker running on port " + std::to_string(TRACKER_PORT));
    Logger::info("Press Ctrl+C to stop");
    
    // Wait for signal
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Cleanup
    Network::cleanup();
    
    return 0;
}
