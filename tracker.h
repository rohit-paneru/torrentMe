#ifndef TRACKER_H
#define TRACKER_H

#include "common.h"
#include "network.h"

// File information structure
struct FileInfo {
    std::string filename;
    std::vector<std::string> peers; // List of "ip:port" strings
};

// Tracker class
class Tracker {
public:
    Tracker(int port = TRACKER_PORT);
    ~Tracker();
    
    // Start the tracker
    bool start();
    
    // Stop the tracker
    void stop();
    
    // Register a file with a peer
    bool registerFile(const std::string& filename, const std::string& peerIp, int peerPort);
    
    // Get peers for a file
    std::vector<std::string> getPeers(const std::string& filename);
    
    // Handle client connection
    void handleClient(socket_t clientSock, const std::string& clientIp, int clientPort);
    
private:
    int port;
    socket_t serverSocket;
    std::atomic<bool> running;
    std::map<std::string, std::vector<std::string>> filePeers;
    std::mutex filePeersMutex;
    std::vector<std::thread> clientThreads;
    
    // Parse client request
    void parseRequest(const std::string& request, const std::string& clientIp, int clientPort, std::string& response);
};

#endif // TRACKER_H