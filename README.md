# MiniTorrent

A minimal BitTorrent client implementation for educational purposes. This project was created as a college project to demonstrate core BitTorrent functionality.

## Features

- **Create Torrent**: Generate `.torrent` files from a file or directory
- **Seed**: Act as a seeder for a torrent file
- **Download**: Download files from a torrent by connecting to seeders

## Requirements

- Python 3.6+
- Dependencies (install with `pip install -r requirements.txt`):
  - bencode.py
  - cryptography

## Installation

1. Clone this repository:

   ```
   git clone https://github.com/yourusername/minitorrent.git
   cd minitorrent
   ```

2. Install dependencies:
   ```
   pip install -r requirements.txt
   ```

## Usage

### Creating a Torrent

```
python minitorrent.py create <input_file_or_directory> <output_torrent_file> [options]
```

Options:

- `-t, --tracker <tracker_url>`: Specify the tracker URL (default: http://example.tracker.com:6969/announce)
- `-p, --piece-length <length>`: Specify piece length in bytes (default: 256KB)

Example:

```
python minitorrent.py create myfile.mp4 myfile.torrent
```

### Seeding a Torrent

```
python minitorrent.py seed <torrent_file> <original_file_or_directory> [options]
```

Options:

- `-p, --port <port>`: Port to listen on (default: 6881)

Example:

```
python minitorrent.py seed myfile.torrent myfile.mp4
```

### Downloading a Torrent

```
python minitorrent.py download <torrent_file> <output_file_or_directory>
```

Example:

```
python minitorrent.py download myfile.torrent downloaded_myfile.mp4
```

## Architecture

The client is split into several components:

1. **Torrent Creator**: Handles creating `.torrent` files with appropriate metadata
2. **Seeder**: Listens for connections and serves file pieces to peers
3. **Downloader**: Connects to seeders and downloads file pieces
4. **Utilities**: Common functions for BitTorrent operations

## Limitations

This is a minimal implementation, so it has several limitations:

- No DHT support (relies on tracker or manual peer specification)
- Limited peer discovery (only connects to peers specified in code for demonstration)
- No support for advanced BitTorrent features like fast resume, superseeding, etc.
- Simple piece selection algorithm (sequential)
- Basic error handling
- Multi-file torrents have limited support

## Testing

To test the client, you can:

1. Create a torrent for a file:

   ```
   python minitorrent.py create myfile.txt myfile.torrent
   ```

2. In one terminal, seed the torrent:

   ```
   python minitorrent.py seed myfile.torrent myfile.txt
   ```

3. In another terminal, download the torrent:
   ```
   python minitorrent.py download myfile.torrent downloaded_myfile.txt
   ```

## Contributing

This project is meant for educational purposes. If you want to contribute:

1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## License

This project is released under the MIT License. See the LICENSE file for details.
