# MiniTorrent Client - Project Documentation

## Table of Contents

1. [Project Overview](#project-overview)
2. [Technologies Used](#technologies-used)
3. [Project Structure](#project-structure)
4. [Core Components](#core-components)
5. [BitTorrent Protocol Implementation](#bittorrent-protocol-implementation)
6. [GUI Implementation](#gui-implementation)
7. [File Handling](#file-handling)
8. [Resume Support](#resume-support)
9. [Security Considerations](#security-considerations)
10. [Limitations and Future Improvements](#limitations-and-future-improvements)

## Project Overview

MiniTorrent is a lightweight BitTorrent client implemented in Python, featuring a graphical user interface for creating torrents, seeding files, and downloading content. The project demonstrates the implementation of the BitTorrent protocol and provides a user-friendly interface for file sharing.

## Technologies Used

### Core Libraries

1. **bencode.py**

   - Purpose: Encoding/decoding BitTorrent's bencoded data format
   - How it works: Implements the BitTorrent encoding format for metadata
   - Key functions:
     - `bencode()`: Converts Python objects to bencoded strings
     - `bdecode()`: Converts bencoded strings back to Python objects

2. **cryptography**

   - Purpose: Cryptographic operations for torrent hashing
   - How it works: Provides secure hash functions (SHA1) for piece verification
   - Key features:
     - Secure hash generation
     - Cryptographic primitives

3. **tkinter**
   - Purpose: GUI framework for the application
   - How it works: Native Python GUI toolkit
   - Key components:
     - Windows and dialogs
     - Widgets and controls
     - Event handling

### Additional Libraries

- **socket**: Network communication
- **threading**: Concurrent operations
- **struct**: Binary data packing/unpacking
- **hashlib**: Hash functions
- **os**: File system operations

## Project Structure

### File Organization

```
minitorrent/
├── minitorrent.py      # Main entry point
├── torrent_utils.py    # Utility functions
├── torrent_creator.py  # Torrent file creation
├── torrent_seeder.py   # Seeding implementation
├── torrent_downloader.py # Downloading implementation
├── torrent_gui.py      # GUI implementation
└── requirements.txt    # Dependencies
```

## Core Components

### 1. Torrent Creator

- **Purpose**: Creates .torrent files from files/directories
- **Key Features**:
  - Single file torrent creation
  - Multi-file torrent support
  - Customizable piece size
  - Tracker URL configuration

### 2. Torrent Seeder

- **Purpose**: Shares files with peers
- **Key Features**:
  - Peer connection handling
  - Piece serving
  - Choke/unchoke management
  - Port configuration

### 3. Torrent Downloader

- **Purpose**: Downloads files from peers
- **Key Features**:
  - Piece downloading
  - Progress tracking
  - Resume support
  - Speed monitoring

## BitTorrent Protocol Implementation

### Handshake Process

1. **Connection Establishment**

   - TCP connection to peer
   - Protocol identifier verification
   - Info hash verification
   - Peer ID exchange

2. **Message Types**
   - Choke (0)
   - Unchoke (1)
   - Interested (2)
   - Not Interested (3)
   - Have (4)
   - Bitfield (5)
   - Request (6)
   - Piece (7)
   - Cancel (8)

### Piece Management

1. **Piece Selection**

   - Rarest piece first
   - Random piece selection
   - Piece verification

2. **Block Management**
   - 16KB block size
   - Multiple outstanding requests
   - Block reassembly

## GUI Implementation

### Features

1. **Tab-based Interface**

   - Create Torrent tab
   - Seed tab
   - Download tab

2. **Automated File Handling**

   - Automatic file naming
   - Progress tracking
   - Speed display
   - Resume support

3. **User Controls**
   - File selection
   - Progress bar
   - Start/Stop buttons
   - Status display

### Threading Model

- Main thread: GUI
- Download thread: Peer communication
- Seeding thread: File serving

## File Handling

### Torrent File Structure

```
{
    "announce": "tracker_url",
    "info": {
        "name": "filename",
        "piece length": 262144,
        "pieces": "piece_hashes",
        "length": file_size
    }
}
```

### Piece Management

1. **Piece Size**

   - Default: 256KB
   - Configurable
   - Last piece may be smaller

2. **File Operations**
   - Random access
   - Piece verification
   - Progress tracking

## Resume Support

### Implementation

1. **Progress Tracking**

   - Bitfield storage
   - Piece verification
   - Progress file management

2. **Resume Process**
   - Progress file loading
   - Piece verification
   - Continuation from last position

## Security Considerations

### Current Implementation

1. **Basic Security**

   - Info hash verification
   - Protocol validation
   - File integrity checks

2. **Limitations**
   - No encryption
   - Basic peer validation
   - Limited DDoS protection

## Limitations and Future Improvements

### Current Limitations

1. **Protocol Support**

   - Basic BitTorrent protocol
   - No DHT support
   - Limited tracker support

2. **Features**
   - Single file downloads
   - Basic peer selection
   - Limited error handling

### Future Improvements

1. **Protocol Enhancements**

   - DHT implementation
   - Multiple tracker support
   - Protocol encryption

2. **Feature Additions**

   - Multi-file support
   - Bandwidth limiting
   - Better peer selection
   - Web seed support

3. **UI Improvements**
   - Dark mode
   - Detailed statistics
   - Peer list view
   - Bandwidth graphs

## Usage Examples

### Creating a Torrent

```python
python minitorrent.py
# Select file/directory
# Set tracker URL
# Click "Create Torrent"
```

### Seeding

```python
# Select torrent file
# Original file auto-detected
# Click "Start Seeding"
```

### Downloading

```python
# Select torrent file
# Output location auto-set
# Click "Start Download"
```

## Development Guidelines

### Code Style

- PEP 8 compliance
- Clear documentation
- Modular design
- Error handling

### Testing

- Unit tests for components
- Integration tests
- Protocol compliance tests

### Contributing

1. Fork the repository
2. Create feature branch
3. Implement changes
4. Submit pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.
