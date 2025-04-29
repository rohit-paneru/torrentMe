import os
import time
from torrent_utils import calculate_piece_hashes, write_torrent_file, calculate_info_hash

def create_torrent(file_path, output_path, tracker_url="http://example.tracker.com:6969/announce", piece_length=262144):
    """
    Create a .torrent file from a given file.
    
    Args:
        file_path: Path to the file to create a torrent for
        output_path: Path where to save the .torrent file
        tracker_url: URL of the tracker
        piece_length: Size of each piece in bytes (default: 256KB)
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")
    
    # Get file information
    file_name = os.path.basename(file_path)
    file_size = os.path.getsize(file_path)
    
    # Calculate piece hashes
    pieces = calculate_piece_hashes(file_path, piece_length)
    
    # Prepare torrent file structure
    info = {
        'name': file_name,
        'piece length': piece_length,
        'pieces': pieces,
        'length': file_size,
    }
    
    # Create the complete torrent dictionary
    torrent = {
        'info': info,
        'announce': tracker_url,
        'creation date': int(time.time()),
        'created by': 'Mini Torrent Client',
    }
    
    # Calculate info hash
    info_hash = calculate_info_hash(info)
    
    # Write the torrent file
    write_torrent_file(torrent, output_path)
    
    return {
        'info_hash': info_hash,
        'piece_length': piece_length,
        'pieces': len(pieces) // 20,  # Each SHA1 hash is 20 bytes
        'file_size': file_size,
        'torrent_path': output_path
    }

def create_multi_file_torrent(directory_path, output_path, tracker_url="http://example.tracker.com:6969/announce", piece_length=262144):
    """
    Create a multi-file .torrent file from a directory.
    
    Args:
        directory_path: Path to the directory to create a torrent for
        output_path: Path where to save the .torrent file
        tracker_url: URL of the tracker
        piece_length: Size of each piece in bytes (default: 256KB)
    """
    if not os.path.isdir(directory_path):
        raise NotADirectoryError(f"Directory not found: {directory_path}")
    
    # Get directory name
    dir_name = os.path.basename(directory_path)
    
    # Collect file information
    files = []
    total_size = 0
    
    for root, _, filenames in os.walk(directory_path):
        for filename in filenames:
            file_path = os.path.join(root, filename)
            rel_path = os.path.relpath(file_path, directory_path)
            file_size = os.path.getsize(file_path)
            
            # Convert Windows path to UNIX-style path for torrent format
            path_components = rel_path.replace('\\', '/').split('/')
            
            files.append({
                'length': file_size,
                'path': path_components
            })
            
            total_size += file_size
    
    # Calculate piece hashes
    # This is more complex for multi-file torrents as we need to read across file boundaries
    # For simplicity, we'll concatenate all files in memory (not efficient for large files)
    pieces = bytearray()
    
    # Read all files in order and calculate pieces
    buffer = bytearray()
    
    for file_info in files:
        file_path = os.path.join(directory_path, '/'.join(file_info['path']))
        with open(file_path, 'rb') as f:
            buffer.extend(f.read())
    
    # Calculate pieces
    for i in range(0, len(buffer), piece_length):
        piece_data = buffer[i:i+piece_length]
        piece_hash = calculate_info_hash(piece_data)
        pieces.extend(piece_hash)
    
    # Prepare torrent file structure
    info = {
        'name': dir_name,
        'piece length': piece_length,
        'pieces': bytes(pieces),
        'files': files
    }
    
    # Create the complete torrent dictionary
    torrent = {
        'info': info,
        'announce': tracker_url,
        'creation date': int(time.time()),
        'created by': 'Mini Torrent Client',
    }
    
    # Calculate info hash
    info_hash = calculate_info_hash(info)
    
    # Write the torrent file
    write_torrent_file(torrent, output_path)
    
    return {
        'info_hash': info_hash,
        'piece_length': piece_length,
        'pieces': len(pieces) // 20,  # Each SHA1 hash is 20 bytes
        'total_size': total_size,
        'files': len(files),
        'torrent_path': output_path
    } 