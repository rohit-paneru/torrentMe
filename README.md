# Mini-Torrent

A simple peer-to-peer file sharing system inspired by BitTorrent. This project consists of two main components:

1. **Tracker**: A central server that keeps track of which peers have which files
2. **Peer**: A client application that can both seed (share) and download files

## System Architecture 

The system follows a simplified BitTorrent-like architecture:

- **Tracker**: Maintains a registry of files and the peers that have them
- **Peers**: Connect to the tracker to register files they're sharing or to find peers that have files they want to download
- **File Transfer**: Direct peer-to-peer file transfer without tracker involvement

## Improved Features

This version includes several improvements over the basic implementation in the project:

1. **Enhanced Error Handling**: Detailed error messages and proper error codes
2. **File Integrity Verification**: Checksum calculation and verification
3. **Improved User Interface**: Better console UI with progress bars
4. **Modular Code Structure**: Organized into reusable components
5. **Cross-Platform Support**: Works on both Windows and POSIX systems
6. **Graceful Shutdown**: Proper cleanup of resources on exit

## Components of Version

### Tracker (tracker.cpp)

The tracker is a central server that:
- Listens on port 8000
- Handles "REGISTER" requests from peers who want to share files
- Handles "GETPEERS" requests from peers looking for files
- Maintains a thread-safe registry of files and their associated peers

### Peer (peer.cpp)

The peer application can:
- Seed files: Share files with other peers
- Download files: Get files from other peers
- Register with the tracker when seeding
- Query the tracker to find peers with desired files
- Verify file integrity using checksums

## How to Use

### Compiling the Project

Using the provided Makefile:

```bash
# Compile both tracker and peer
make

# Compile only tracker
make tracker

# Compile only peer
make peer

# Clean build files
make clean
```

Manual compilation:

```bash
# Compile the tracker
g++ -o tracker tracker.cpp common.cpp network.cpp fileutils.cpp -std=c++11 -pthread

# Compile the peer
g++ -o peer peer.cpp common.cpp network.cpp fileutils.cpp -std=c++11 -pthread
```

### Running the Tracker

```bash
./tracker
```

The tracker will start listening on port 8000 and display log messages.

### Running a Peer

```bash
./peer
```

The peer application will present a menu with options:

1. **Seed a file**: Share a file with other peers to go ahead
   - You'll need to provide the file path and a port number to serve on
   - The peer will register with the tracker and start listening for download requests
   - File integrity is verified with checksums

2. **Download a file**: Get a file from another peer
   - You'll need to provide the filename to search for and a destination path
   - The peer will query the tracker for available sources
   - You can select which peer to download from
   - Progress bar shows download status
   - File integrity is verified after download

3. **Exit**: Quit the application with proper cleanup

## Technical Details

- Written in C++ with standard networking libraries
- Uses TCP sockets for reliable data transfer
- Implements a simple protocol for tracker-peer communication
- Supports concurrent connections with multi-threading
- File transfers occur in chunks of 1024 bytes
- Cross-platform socket handling (Windows/POSIX)
- Logging system with different severity levels
- Progress tracking for file transfers

## Project Structure

- **common.h/cpp**: Common utilities and definitions
- **network.h/cpp**: Network communication utilities
- **fileutils.h/cpp**: File handling utilities
- **tracker.h/cpp**: Tracker implementation
- **peer.h/cpp**: Peer implementation
- **Makefile**: Build configuration

## Requirements

- C++ compiler with C++11 support
- pthread library (Linux/macOS)
- Windows: Windows Sockets 2 library (ws2_32)

## Future Improvements

Potential areas for further enhancement of project:

- Downloading from multiple peers simultaneously
- More robust file integrity verification (SHA-256)
- NAT traversal capabilities
- Graphical user interface
- Persistent storage for tracker data
