import socket
import threading
import struct
import random
import time
import os
import math
import pickle
from torrent_utils import read_torrent_file, calculate_info_hash, get_peer_id, save_piece_to_file, create_bitfield, set_bit, check_bit

class Downloader:
    def __init__(self, torrent_path, download_path, max_connections=5):
        """
        Initialize a downloader for a torrent file.
        
        Args:
            torrent_path: Path to the .torrent file
            download_path: Path where to save the downloaded file
            max_connections: Maximum number of simultaneous connections
        """
        self.torrent_path = torrent_path
        self.download_path = download_path
        self.max_connections = max_connections
        self.running = False
        self.torrent_data = None
        self.info_hash = None
        self.piece_length = None
        self.num_pieces = None
        self.total_length = None
        self.downloaded_pieces = 0
        self.peer_id = get_peer_id()
        self.peers = []
        self.active_peers = {}
        self.bitfield = None
        self.lock = threading.Lock()
        self.piece_hashes = []
        self.progress_file = f"{download_path}.progress"
        
    def start(self):
        """Start downloading the torrent."""
        if self.running:
            print("Downloader is already running")
            return
            
        # Load torrent file
        self.torrent_data = read_torrent_file(self.torrent_path)
        
        # Calculate info hash
        self.info_hash = calculate_info_hash(self.torrent_data['info'])
        
        # Get piece length
        self.piece_length = self.torrent_data['info']['piece length']
        
        # Get file length
        if 'length' in self.torrent_data['info']:
            # Single file torrent
            self.total_length = self.torrent_data['info']['length']
            self.multi_file = False
        else:
            # Multi-file torrent
            self.total_length = sum(file_info['length'] for file_info in self.torrent_data['info']['files'])
            self.multi_file = True
            # We don't fully support multi-file torrents in this mini client
            print("Warning: Multi-file torrents not fully supported")
        
        # Calculate number of pieces
        pieces_string = self.torrent_data['info']['pieces']
        self.num_pieces = len(pieces_string) // 20  # Each SHA1 hash is 20 bytes
        
        # Extract piece hashes
        for i in range(self.num_pieces):
            self.piece_hashes.append(pieces_string[i*20:(i+1)*20])
        
        # Try to load progress from file
        if os.path.exists(self.progress_file):
            try:
                with open(self.progress_file, 'rb') as f:
                    saved_data = pickle.load(f)
                    if saved_data['info_hash'] == self.info_hash:
                        self.bitfield = saved_data['bitfield']
                        self.downloaded_pieces = sum(1 for i in range(self.num_pieces) if check_bit(self.bitfield, i))
                        print(f"Resuming download from {self.downloaded_pieces}/{self.num_pieces} pieces")
                    else:
                        print("Progress file is for a different torrent, starting fresh")
                        self.bitfield = create_bitfield(self.num_pieces)
            except Exception as e:
                print(f"Error loading progress file: {e}")
                self.bitfield = create_bitfield(self.num_pieces)
        else:
            self.bitfield = create_bitfield(self.num_pieces)
        
        # Initialize file with zeros if it doesn't exist
        if not os.path.exists(self.download_path):
            with open(self.download_path, 'wb') as f:
                f.write(b'\0' * self.total_length)
        
        # Add some hardcoded peers for testing (normally we'd get this from a tracker)
        # In a real implementation, we'd connect to a tracker to get peer list
        # Format: (ip, port)
        self.peers = [('127.0.0.1', 6881)]  # Example peer (localhost)
        
        self.running = True
        print(f"Downloader started")
        print(f"Info hash: {self.info_hash.hex()}")
        print(f"Piece length: {self.piece_length}")
        print(f"Number of pieces: {self.num_pieces}")
        print(f"Total length: {self.total_length}")
        
        # Connect to peers
        for peer in self.peers:
            self._connect_to_peer(peer)
        
    def stop(self):

        if not self.running:
            print("Downloader is not running")
            return
            
        self.running = False
        
        try:
            with open(self.progress_file, 'wb') as f:
                pickle.dump({
                    'info_hash': self.info_hash,
                    'bitfield': self.bitfield
                }, f)
        except Exception as e:
            print(f"Error saving progress: {e}")
        
        # Close all peer connections
        for peer_id, peer_data in list(self.active_peers.items()):
            peer_data['socket'].close()
            
        print("Downloader stopped")
        
    def _connect_to_peer(self, peer):
        
        if len(self.active_peers) >= self.max_connections:
            return
            
        ip, port = peer
        
        try:
            # Create socket
            peer_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            peer_socket.settimeout(10)
            
            # Connect to peer
            peer_socket.connect((ip, port))
            
            # Send handshake
            handshake = bytes([19]) + b'BitTorrent protocol' + (b'\x00' * 8) + self.info_hash + self.peer_id
            peer_socket.send(handshake)
            
            # Receive handshake
            response = peer_socket.recv(68)
            if len(response) != 68 or response[0] != 19 or response[1:20] != b'BitTorrent protocol':
                print(f"Invalid handshake from {ip}:{port}")
                peer_socket.close()
                return
                
            # Extract info hash and peer ID from handshake
            peer_info_hash = response[28:48]
            peer_id = response[48:68]
            
            # Verify info hash
            if peer_info_hash != self.info_hash:
                print(f"Invalid info hash from {ip}:{port}")
                peer_socket.close()
                return
                
            # Add peer to active peers
            with self.lock:
                self.active_peers[peer_id] = {
                    'socket': peer_socket,
                    'address': (ip, port),
                    'am_choking': True,        # We are choking the peer
                    'am_interested': False,    # We are interested in the peer
                    'peer_choking': True,      # Peer is choking us
                    'peer_interested': False,  # Peer is interested in us
                    'have': create_bitfield(self.num_pieces),
                    'requested_pieces': set()
                }
            
            # Start communication thread
            peer_thread = threading.Thread(target=self._handle_peer, args=(peer_id,))
            peer_thread.daemon = True
            peer_thread.start()
            
            print(f"Connected to peer {ip}:{port}")
            
        except Exception as e:
            print(f"Error connecting to peer {ip}:{port}: {e}")
            
    def _handle_peer(self, peer_id):
        """Handle communication with a peer."""
        peer_data = self.active_peers.get(peer_id)
        if not peer_data:
            return
            
        peer_socket = peer_data['socket']
        
        try:
            # Send interested message
            self._send_interested(peer_socket)
            peer_data['am_interested'] = True
            
            # Handle messages from peer
            while self.running and peer_id in self.active_peers:
                # Receive message length
                length_prefix = peer_socket.recv(4)
                if not length_prefix or len(length_prefix) < 4:
                    break
                    
                length = struct.unpack(">I", length_prefix)[0]
                
                # Keep-alive message
                if length == 0:
                    continue
                    
                # Receive message
                message = peer_socket.recv(length)
                if not message or len(message) < length:
                    break
                    
                # Process message
                self._handle_message(peer_id, message[0], message[1:])
                
                # Request pieces if we're unchoked
                if not peer_data['peer_choking']:
                    self._request_pieces(peer_id)
                    
        except Exception as e:
            print(f"Error handling peer {peer_data['address']}: {e}")
            
        finally:
            # Clean up
            with self.lock:
                if peer_id in self.active_peers:
                    del self.active_peers[peer_id]
                    
            try:
                peer_socket.close()
            except:
                pass
                
            print(f"Connection closed with {peer_data['address']}")
            
    def _handle_message(self, peer_id, message_id, payload):
        """Handle a BitTorrent protocol message."""
        peer_data = self.active_peers[peer_id]
        
        # Choke (0)
        if message_id == 0:
            peer_data['peer_choking'] = True
            print(f"Peer {peer_data['address']} choked us")
            
        # Unchoke (1)
        elif message_id == 1:
            peer_data['peer_choking'] = False
            print(f"Peer {peer_data['address']} unchoked us")
            
        # Interested (2)
        elif message_id == 2:
            peer_data['peer_interested'] = True
            
        # Not interested (3)
        elif message_id == 3:
            peer_data['peer_interested'] = False
            
        # Have (4)
        elif message_id == 4:
            piece_index = struct.unpack(">I", payload)[0]
            set_bit(peer_data['have'], piece_index)
            
            # If we don't have this piece yet, express interest
            with self.lock:
                if not check_bit(self.bitfield, piece_index):
                    if not peer_data['am_interested']:
                        self._send_interested(peer_data['socket'])
                        peer_data['am_interested'] = True
            
        # Bitfield (5)
        elif message_id == 5:
            peer_data['have'] = payload
            
            # Check if peer has pieces we need
            for i in range(self.num_pieces):
                if check_bit(peer_data['have'], i) and not check_bit(self.bitfield, i):
                    if not peer_data['am_interested']:
                        self._send_interested(peer_data['socket'])
                        peer_data['am_interested'] = True
                        break
            
        # Request (6)
        elif message_id == 6:
            # We don't handle requests in this simple client
            pass
            
        # Piece (7)
        elif message_id == 7:
            # The first 8 bytes of the payload contain the piece index and offset
            if len(payload) < 8:
                return
                
            index, begin = struct.unpack(">II", payload[:8])
            block = payload[8:]
            
            # Save piece to file
            with self.lock:
                piece_offset = index * self.piece_length + begin
                try:
                    save_piece_to_file(self.download_path, block, piece_offset)
                except Exception as e:
                    print(f"Error saving piece {index} at offset {piece_offset}: {e}")
                    return
                
                # If we've completed a piece, update our bitfield
                if self._check_piece_complete(index):
                    set_bit(self.bitfield, index)
                    self.downloaded_pieces += 1
                    self._send_have(index)
                    
                    # Remove from requested pieces
                    if index in peer_data['requested_pieces']:
                        peer_data['requested_pieces'].remove(index)
                    
                    # Print progress
                    progress = (self.downloaded_pieces / self.num_pieces) * 100
                    print(f"Downloaded piece {index} ({progress:.2f}% complete)")
                    
                    # Check if download is complete
                    if self.downloaded_pieces == self.num_pieces:
                        print("Download complete!")
                        self.stop()
            
        # Cancel (8)
        elif message_id == 8:
            # We don't handle cancel messages in this simple client
            pass
            
    def _send_interested(self, socket):
        """Send an interested message to a peer."""
        message = struct.pack(">IB", 1, 2)
        socket.send(message)
        
    def _send_have(self, piece_index):
        """Send have messages to all peers."""
        message = struct.pack(">IBI", 5, 4, piece_index)
        
        for peer_id, peer_data in list(self.active_peers.items()):
            try:
                peer_data['socket'].send(message)
            except:
                pass
                
    def _request_pieces(self, peer_id):
        """Request pieces from a peer."""
        peer_data = self.active_peers[peer_id]
        
        # Don't request if we're being choked
        if peer_data['peer_choking']:
            return
            
        # Maximum number of outstanding requests
        max_requests = 5
        
        # Don't request more than max_requests at a time
        if len(peer_data['requested_pieces']) >= max_requests:
            return
            
        # Find pieces to request
        for i in range(self.num_pieces):
            # Check if peer has piece and we don't have it and we haven't requested it yet
            if (check_bit(peer_data['have'], i) and 
                not check_bit(self.bitfield, i) and
                i not in peer_data['requested_pieces']):
                
                # Request the piece
                self._request_piece(peer_id, i)
                peer_data['requested_pieces'].add(i)
                
                # Don't request more than max_requests at a time
                if len(peer_data['requested_pieces']) >= max_requests:
                    break
                    
    def _request_piece(self, peer_id, piece_index):
        """Request a piece from a peer."""
        peer_data = self.active_peers[peer_id]
        
        # Calculate piece size
        if piece_index == self.num_pieces - 1:
            # Last piece might be smaller
            piece_size = self.total_length - (piece_index * self.piece_length)
        else:
            piece_size = self.piece_length
            
        # Request in blocks of 16KB
        block_size = 16384  # 16KB
        
        # Calculate number of blocks
        num_blocks = math.ceil(piece_size / block_size)
        
        for block_index in range(num_blocks):
            begin = block_index * block_size
            
            # Last block might be smaller
            if block_index == num_blocks - 1:
                length = piece_size - (block_index * block_size)
            else:
                length = block_size
                
            # Send request message
            message = struct.pack(">IBIII", 13, 6, piece_index, begin, length)
            
            try:
                peer_data['socket'].send(message)
            except:
                # Handle socket errors
                if peer_id in self.active_peers:
                    del self.active_peers[peer_id]
                return
                
    def _check_piece_complete(self, piece_index):
       
        # For simplicity, we'll assume a piece is complete once we've received data for it
        # In a real implementation, we would verify the hash of the piece
        return True 