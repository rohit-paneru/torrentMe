import os
import hashlib
import bencode
import socket
import struct
import random
import math
from collections import defaultdict

def read_torrent_file(torrent_path):
    """Read and decode a .torrent file."""
    with open(torrent_path, 'rb') as f:
        return bencode.bdecode(f.read())

def write_torrent_file(data, output_path):
    """Encode and write data to a .torrent file."""
    with open(output_path, 'wb') as f:
        f.write(bencode.bencode(data))

def calculate_piece_hashes(file_path, piece_length):
    """Calculate SHA1 hashes for all pieces of a file."""
    piece_hashes = bytearray()
    
    with open(file_path, 'rb') as f:
        while True:
            piece_data = f.read(piece_length)
            if not piece_data:
                break
                
            sha1_hash = hashlib.sha1(piece_data).digest()
            piece_hashes.extend(sha1_hash)
            
    return bytes(piece_hashes)

def calculate_info_hash(info_dict):
    """Calculate the info hash for a torrent."""
    encoded_info = bencode.bencode(info_dict)
    return hashlib.sha1(encoded_info).digest()

def get_file_size(file_path):
    """Get file size in bytes."""
    return os.path.getsize(file_path)

def get_peer_id():
    """Generate a peer ID."""
    peer_id = '-PY0001-'
    for _ in range(12):
        peer_id += str(random.randint(0, 9))
    return peer_id.encode()

def split_file_into_pieces(file_path, piece_length):
    """Split a file into pieces of specified length."""
    pieces = []
    with open(file_path, 'rb') as f:
        while True:
            piece_data = f.read(piece_length)
            if not piece_data:
                break
            pieces.append(piece_data)
    return pieces

def get_piece_from_file(file_path, piece_length, piece_index):
    """Get a specific piece from a file."""
    with open(file_path, 'rb') as f:
        f.seek(piece_index * piece_length)
        return f.read(piece_length)

def save_piece_to_file(file_path, piece_data, piece_offset):
    """Save a piece to a specific location in a file."""
    with open(file_path, 'r+b') as f:
        f.seek(piece_offset)
        f.write(piece_data)

def create_bitfield(num_pieces):
    """Create a bitfield of zeros."""
    length = math.ceil(num_pieces / 8)
    return bytearray(length)

def set_bit(bitfield, index):
    """Set a bit in the bitfield."""
    byte_index = index // 8
    bit_index = 7 - (index % 8)
    bitfield[byte_index] |= (1 << bit_index)

def check_bit(bitfield, index):
    """Check if a bit is set in the bitfield."""
    byte_index = index // 8
    bit_index = 7 - (index % 8)
    return (bitfield[byte_index] & (1 << bit_index)) != 0 