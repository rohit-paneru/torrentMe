import socket
import threading
import struct
import time
import os
from torrent_utils import read_torrent_file, calculate_info_hash, get_peer_id, get_piece_from_file, create_bitfield, set_bit

class Seeder:
    def __init__(self, torrent_path, file_path, listen_port=6881):
        """
        Initialize a seeder for a torrent file.
        
        Args:
            torrent_path: Path to the .torrent file
            file_path: Path to the original file being shared
            listen_port: Port to listen for incoming connections
        """
        self.torrent_path = torrent_path
        self.file_path = file_path
        self.listen_port = listen_port
        self.running = False
        self.peers = {}
        self.torrent_data = None
        self.info_hash = None
        self.piece_length = None
        self.num_pieces = None
        self.peer_id = get_peer_id()
        self.server_socket = None
        
    def start(self):
        """Start seeding the torrent."""
        if self.running:
            print("Seeder is already running")
            return
            
        # Load torrent file
        self.torrent_data = read_torrent_file(self.torrent_path)
        
        # Calculate info hash
        self.info_hash = calculate_info_hash(self.torrent_data['info'])
        
        # Get piece length
        self.piece_length = self.torrent_data['info']['piece length']
        
        # Calculate number of pieces
        pieces_string = self.torrent_data['info']['pieces']
        self.num_pieces = len(pieces_string) // 20  # Each SHA1 hash is 20 bytes
        
        # Create bitfield (we have all pieces)
        self.bitfield = create_bitfield(self.num_pieces)
        for i in range(self.num_pieces):
            set_bit(self.bitfield, i)
        
        # Verify file exists
        if not os.path.exists(self.file_path):
            raise FileNotFoundError(f"File not found: {self.file_path}")
            
        # Start listening for connections
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind(('0.0.0.0', self.listen_port))
        self.server_socket.listen(5)
        
        self.running = True
        print(f"Seeder started on port {self.listen_port}")
        print(f"Info hash: {self.info_hash.hex()}")
        print(f"Piece length: {self.piece_length}")
        print(f"Number of pieces: {self.num_pieces}")
        
        # Start accepting connections in a separate thread
        accept_thread = threading.Thread(target=self._accept_connections)
        accept_thread.daemon = True
        accept_thread.start()
        
    def stop(self):
        """Stop seeding the torrent."""
        if not self.running:
            print("Seeder is not running")
            return
            
        self.running = False
        
        # Close all peer connections
        for peer_id, peer_data in self.peers.items():
            peer_data['socket'].close()
            
        # Close server socket
        if self.server_socket:
            self.server_socket.close()
            
        print("Seeder stopped")
        
    def _accept_connections(self):
        """Accept incoming connections from peers."""
        while self.running:
            try:
                client_socket, address = self.server_socket.accept()
                print(f"New connection from {address}")
                
                # Handle handshake in a new thread
                client_thread = threading.Thread(target=self._handle_client, args=(client_socket, address))
                client_thread.daemon = True
                client_thread.start()
                
            except Exception as e:
                if self.running:
                    print(f"Error accepting connection: {e}")
                
    def _handle_client(self, client_socket, address):
        """Handle communication with a peer."""
        try:
            # Receive handshake
            handshake = client_socket.recv(68)
            if len(handshake) != 68 or handshake[0] != 19 or handshake[1:20] != b'BitTorrent protocol':
                print(f"Invalid handshake from {address}")
                client_socket.close()
                return
                
            # Extract info hash and peer ID from handshake
            peer_info_hash = handshake[28:48]
            peer_id = handshake[48:68]
            
            # Verify info hash
            if peer_info_hash != self.info_hash:
                print(f"Invalid info hash from {address}")
                client_socket.close()
                return
                
            # Send handshake response
            response = bytes([19]) + b'BitTorrent protocol' + (b'\x00' * 8) + self.info_hash + self.peer_id
            client_socket.send(response)
            
            # Add peer to connected peers
            self.peers[peer_id] = {
                'socket': client_socket,
                'address': address,
                'interested': False,
                'choked': True,
                'have': create_bitfield(self.num_pieces)
            }
            
            # Send bitfield message (we have all pieces)
            self._send_bitfield(client_socket)
            
            # Handle messages from peer
            self._message_loop(client_socket, peer_id)
            
        except Exception as e:
            print(f"Error handling client {address}: {e}")
            
        finally:
            # Clean up
            if peer_id in self.peers:
                del self.peers[peer_id]
                
            try:
                client_socket.close()
            except:
                pass
                
            print(f"Connection closed with {address}")
            
    def _message_loop(self, client_socket, peer_id):
        """Handle BitTorrent messages from a peer."""
        while self.running and peer_id in self.peers:
            try:
                # Receive message length
                length_prefix = client_socket.recv(4)
                if not length_prefix:
                    break
                    
                length = struct.unpack(">I", length_prefix)[0]
                
                # Keep-alive message
                if length == 0:
                    continue
                    
                # Receive message
                message = client_socket.recv(length)
                if not message:
                    break
                    
                # Process message
                self._handle_message(client_socket, peer_id, message[0], message[1:])
                
            except Exception as e:
                print(f"Error in message loop: {e}")
                break
                
    def _handle_message(self, client_socket, peer_id, message_id, payload):
        """Handle a BitTorrent protocol message."""
        peer_data = self.peers[peer_id]
        
        # Choke (0)
        if message_id == 0:
            pass  # We don't respond to choke messages as a seeder
            
        # Unchoke (1)
        elif message_id == 1:
            pass  # We don't respond to unchoke messages as a seeder
            
        # Interested (2)
        elif message_id == 2:
            peer_data['interested'] = True
            # Unchoke the peer
            self._send_unchoke(client_socket)
            peer_data['choked'] = False
            
        # Not interested (3)
        elif message_id == 3:
            peer_data['interested'] = False
            
        # Have (4)
        elif message_id == 4:
            piece_index = struct.unpack(">I", payload)[0]
            set_bit(peer_data['have'], piece_index)
            
        # Bitfield (5)
        elif message_id == 5:
            peer_data['have'] = payload
            
        # Request (6)
        elif message_id == 6:
            if not peer_data['choked']:
                index, begin, length = struct.unpack(">III", payload)
                self._send_piece(client_socket, index, begin, length)
                
        # Piece (7)
        elif message_id == 7:
            pass  # We don't expect piece messages as a seeder
            
        # Cancel (8)
        elif message_id == 8:
            pass  # We currently don't handle cancel messages
            
    def _send_bitfield(self, client_socket):
        """Send a bitfield message to a peer."""
        message = struct.pack(">IB", 1 + len(self.bitfield), 5) + self.bitfield
        client_socket.send(message)
        
    def _send_unchoke(self, client_socket):
        """Send an unchoke message to a peer."""
        message = struct.pack(">IB", 1, 1)
        client_socket.send(message)
        
    def _send_piece(self, client_socket, index, begin, length):
        """Send a piece to a peer."""
        # Get piece data from file
        piece_data = get_piece_from_file(self.file_path, self.piece_length, index)
        
        # Extract requested block
        if begin + length > len(piece_data):
            # Handle request that goes beyond piece boundary
            block = piece_data[begin:]
        else:
            block = piece_data[begin:begin+length]
            
        # Construct piece message
        message_length = 9 + len(block)  # 1 (message ID) + 4 (index) + 4 (begin) + block length
        message = struct.pack(">IBI", message_length, 7, index) + struct.pack(">I", begin) + block
        
        # Send message
        client_socket.send(message) 