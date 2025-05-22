#ifndef PEER_H
#define PEER_H

#include "common.h"
#include "network.h"
#include "fileutils.h"

// Peer class
class Peer {
public:
    Peer();
    ~Peer();
    
    // Seed a file
    bool seedFile(const std::string& filepath, int port);
    
    // Download a file
    bool downloadFile(const std::string& filename, const std::string& destPath);
    
    // Stop seeding
    void stopSeeding();
    
    // Register with tracker
    bool registerWithTracker(const std::string& filename, int port);
    
    // Get peers from tracker
    std::vector<std::string> getPeersFromTracker(const std::string& filename);
    
    // Handle download request
    void handleDownloadRequest(socket_t clientSock);
    
private:
    std::string currentSeedingFile;
    int seedingPort;
    socket_t serverSocket;
    std::atomic<bool> seeding;
    std::thread seederThread;
    
    // Start seeder thread
    void startSeederThread(const std::string& filepath, int port);
};

#endif // PEER_H