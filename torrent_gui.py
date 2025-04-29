import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import threading
import time
from torrent_creator import create_torrent, create_multi_file_torrent
from torrent_seeder import Seeder
from torrent_downloader import Downloader
from torrent_utils import read_torrent_file
import os

class TorrentGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("MiniTorrent Client")
        self.root.geometry("600x400")
        
        # Create notebook for tabs
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(expand=True, fill='both', padx=5, pady=5)
        
        # Create tabs
        self.create_tab = ttk.Frame(self.notebook)
        self.seed_tab = ttk.Frame(self.notebook)
        self.download_tab = ttk.Frame(self.notebook)
        
        self.notebook.add(self.create_tab, text='Create Torrent')
        self.notebook.add(self.seed_tab, text='Seed')
        self.notebook.add(self.download_tab, text='Download')
        
        # Initialize variables
        self.seeder = None
        self.downloader = None
        self.download_thread = None
        self.seed_thread = None
        self.last_downloaded = 0
        self.last_time = 0
        self.download_speed = 0
        
        # Setup tabs
        self.setup_create_tab()
        self.setup_seed_tab()
        self.setup_download_tab()
        
    def setup_create_tab(self):
        # Input file/directory selection
        ttk.Label(self.create_tab, text="Select File/Directory to Create Torrent:").pack(pady=5)
        self.input_frame = ttk.Frame(self.create_tab)
        self.input_frame.pack(fill='x', padx=5)
        self.input_path = tk.StringVar()
        ttk.Entry(self.input_frame, textvariable=self.input_path).pack(side='left', expand=True, fill='x')
        ttk.Button(self.input_frame, text="Browse", command=self.browse_input).pack(side='right', padx=5)
        
        # Output torrent file selection (read-only)
        ttk.Label(self.create_tab, text="Torrent File (Auto-generated):").pack(pady=5)
        self.output_frame = ttk.Frame(self.create_tab)
        self.output_frame.pack(fill='x', padx=5)
        self.output_path = tk.StringVar()
        ttk.Entry(self.output_frame, textvariable=self.output_path, state='readonly').pack(side='left', expand=True, fill='x')
        
        # Tracker URL
        ttk.Label(self.create_tab, text="Tracker URL:").pack(pady=5)
        self.tracker_url = tk.StringVar(value="http://example.tracker.com:6969/announce")
        ttk.Entry(self.create_tab, textvariable=self.tracker_url).pack(fill='x', padx=5)
        
        # Piece length
        ttk.Label(self.create_tab, text="Piece Length (bytes):").pack(pady=5)
        self.piece_length = tk.StringVar(value="262144")
        ttk.Entry(self.create_tab, textvariable=self.piece_length).pack(fill='x', padx=5)
        
        # Create button
        ttk.Button(self.create_tab, text="Create Torrent", command=self.create_torrent).pack(pady=10)
        
    def setup_seed_tab(self):
        # Torrent file selection
        ttk.Label(self.seed_tab, text="Select Torrent File to Seed:").pack(pady=5)
        self.seed_torrent_frame = ttk.Frame(self.seed_tab)
        self.seed_torrent_frame.pack(fill='x', padx=5)
        self.seed_torrent_path = tk.StringVar()
        ttk.Entry(self.seed_torrent_frame, textvariable=self.seed_torrent_path).pack(side='left', expand=True, fill='x')
        ttk.Button(self.seed_torrent_frame, text="Browse", command=self.browse_seed_torrent).pack(side='right', padx=5)
        
        # Original file selection (read-only)
        ttk.Label(self.seed_tab, text="Original File (Auto-detected):").pack(pady=5)
        self.seed_file_frame = ttk.Frame(self.seed_tab)
        self.seed_file_frame.pack(fill='x', padx=5)
        self.seed_file_path = tk.StringVar()
        ttk.Entry(self.seed_file_frame, textvariable=self.seed_file_path, state='readonly').pack(side='left', expand=True, fill='x')
        
        # Port
        ttk.Label(self.seed_tab, text="Port:").pack(pady=5)
        self.seed_port = tk.StringVar(value="6881")
        ttk.Entry(self.seed_tab, textvariable=self.seed_port).pack(fill='x', padx=5)
        
        # Status
        self.seed_status = tk.StringVar(value="Not running")
        ttk.Label(self.seed_tab, textvariable=self.seed_status).pack(pady=5)
        
        # Start/Stop button
        self.seed_button = ttk.Button(self.seed_tab, text="Start Seeding", command=self.toggle_seeding)
        self.seed_button.pack(pady=10)
        
    def setup_download_tab(self):
        # Torrent file selection
        ttk.Label(self.download_tab, text="Select Torrent File to Download:").pack(pady=5)
        self.download_torrent_frame = ttk.Frame(self.download_tab)
        self.download_torrent_frame.pack(fill='x', padx=5)
        self.download_torrent_path = tk.StringVar()
        ttk.Entry(self.download_torrent_frame, textvariable=self.download_torrent_path).pack(side='left', expand=True, fill='x')
        ttk.Button(self.download_torrent_frame, text="Browse", command=self.browse_download_torrent).pack(side='right', padx=5)
        
        # Download directory selection
        ttk.Label(self.download_tab, text="Select Download Directory:").pack(pady=5)
        self.download_dir_frame = ttk.Frame(self.download_tab)
        self.download_dir_frame.pack(fill='x', padx=5)
        self.download_dir = tk.StringVar()
        ttk.Entry(self.download_dir_frame, textvariable=self.download_dir).pack(side='left', expand=True, fill='x')
        ttk.Button(self.download_dir_frame, text="Browse", command=self.browse_download_dir).pack(side='right', padx=5)
        
        # Output file name (editable)
        ttk.Label(self.download_tab, text="Output File Name:").pack(pady=5)
        self.download_output_frame = ttk.Frame(self.download_tab)
        self.download_output_frame.pack(fill='x', padx=5)
        self.download_output_path = tk.StringVar()
        ttk.Entry(self.download_output_frame, textvariable=self.download_output_path).pack(side='left', expand=True, fill='x')
        
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(self.download_tab, variable=self.progress_var, maximum=100)
        self.progress_bar.pack(fill='x', padx=5, pady=5)
        
        # Status
        self.download_status = tk.StringVar(value="Not running")
        ttk.Label(self.download_tab, textvariable=self.download_status).pack(pady=5)
        
        # Speed
        self.speed_var = tk.StringVar(value="Speed: 0 B/s")
        ttk.Label(self.download_tab, textvariable=self.speed_var).pack(pady=5)
        
        # Resume checkbox
        self.resume_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(self.download_tab, text="Resume if possible", variable=self.resume_var).pack(pady=5)
        
        # Start/Stop button
        self.download_button = ttk.Button(self.download_tab, text="Start Download", command=self.toggle_download)
        self.download_button.pack(pady=10)
        
    def browse_input(self):
        path = filedialog.askopenfilename() if not os.path.isdir(self.input_path.get()) else filedialog.askdirectory()
        if path:
            self.input_path.set(path)
            # Auto-set output torrent file name
            if os.path.isdir(path):
                base_name = os.path.basename(path)
            else:
                base_name = os.path.splitext(os.path.basename(path))[0]
            output_path = os.path.join(os.path.dirname(path), f"{base_name}.torrent")
            self.output_path.set(output_path)
            
    def browse_seed_torrent(self):
        path = filedialog.askopenfilename(filetypes=[("Torrent files", "*.torrent")])
        if path:
            self.seed_torrent_path.set(path)
            # Auto-set original file path
            base_name = os.path.splitext(os.path.basename(path))[0]
            original_file = os.path.join(os.path.dirname(path), base_name)
            if os.path.exists(original_file):
                self.seed_file_path.set(original_file)
            else:
                # Try with .txt extension
                original_file = f"{original_file}.txt"
                if os.path.exists(original_file):
                    self.seed_file_path.set(original_file)
            
    def browse_download_dir(self):
        """Browse for download directory."""
        path = filedialog.askdirectory()
        if path:
            self.download_dir.set(path)
            # Update output path if torrent is selected
            if self.download_torrent_path.get():
                self.update_output_path()
                
    def update_output_path(self):
        """Update output path based on selected directory and torrent file."""
        try:
            torrent_data = read_torrent_file(self.download_torrent_path.get())
            original_name = torrent_data['info']['name']
            original_ext = os.path.splitext(original_name)[1]
            
            # Create new name with "_downloaded" suffix
            base_name = os.path.splitext(original_name)[0]
            new_name = f"{base_name}_downloaded{original_ext}"
            
            # Set the output path in the selected directory
            output_path = os.path.join(self.download_dir.get() or os.path.dirname(self.download_torrent_path.get()), new_name)
            self.download_output_path.set(output_path)
            
            # Update status to show file type
            file_type = self.get_file_type(original_ext)
            self.download_status.set(f"File type: {file_type}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to read torrent file: {e}")
            self.download_status.set("Error reading torrent file")
            
    def browse_download_torrent(self):
        path = filedialog.askopenfilename(filetypes=[("Torrent files", "*.torrent")])
        if path:
            self.download_torrent_path.set(path)
            # Update output path
            self.update_output_path()
            
    def get_file_type(self, extension):
        """Get human-readable file type from extension."""
        extension = extension.lower()
        file_types = {
            # Text files
            '.txt': 'Text Document',
            '.doc': 'Word Document',
            '.docx': 'Word Document',
            '.pdf': 'PDF Document',
            
            # Images
            '.jpg': 'JPEG Image',
            '.jpeg': 'JPEG Image',
            '.png': 'PNG Image',
            '.gif': 'GIF Image',
            '.bmp': 'Bitmap Image',
            
            # Audio
            '.mp3': 'MP3 Audio',
            '.wav': 'WAV Audio',
            '.ogg': 'OGG Audio',
            '.flac': 'FLAC Audio',
            
            # Video
            '.mp4': 'MP4 Video',
            '.avi': 'AVI Video',
            '.mkv': 'MKV Video',
            '.mov': 'QuickTime Video',
            
            # Archives
            '.zip': 'ZIP Archive',
            '.rar': 'RAR Archive',
            '.7z': '7-Zip Archive',
            '.tar': 'TAR Archive',
            
            # Code
            '.py': 'Python Script',
            '.java': 'Java Source',
            '.cpp': 'C++ Source',
            '.html': 'HTML Document',
            '.css': 'CSS Stylesheet',
            '.js': 'JavaScript File',
            
            # Other common types
            '.exe': 'Executable',
            '.dll': 'Dynamic Link Library',
            '.iso': 'ISO Image',
            '.bin': 'Binary File',
        }
        return file_types.get(extension, 'Unknown File Type')
        
    def create_torrent(self):
        try:
            input_path = self.input_path.get()
            output_path = self.output_path.get()
            tracker_url = self.tracker_url.get()
            piece_length = int(self.piece_length.get())
            
            if not input_path or not output_path:
                messagebox.showerror("Error", "Please select input and output paths")
                return
                
            if os.path.isdir(input_path):
                result = create_multi_file_torrent(input_path, output_path, tracker_url, piece_length)
            else:
                result = create_torrent(input_path, output_path, tracker_url, piece_length)
                
            messagebox.showinfo("Success", f"Torrent created successfully!\nInfo hash: {result['info_hash'].hex()}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to create torrent: {str(e)}")
            
    def toggle_seeding(self):
        if self.seeder is None:
            try:
                torrent_path = self.seed_torrent_path.get()
                file_path = self.seed_file_path.get()
                port = int(self.seed_port.get())
                
                if not torrent_path or not file_path:
                    messagebox.showerror("Error", "Please select torrent file and original file")
                    return
                    
                self.seeder = Seeder(torrent_path, file_path, port)
                self.seed_thread = threading.Thread(target=self.seed_thread_func)
                self.seed_thread.daemon = True
                self.seed_thread.start()
                
                self.seed_button.config(text="Stop Seeding")
                self.seed_status.set("Seeding...")
                
            except Exception as e:
                messagebox.showerror("Error", f"Failed to start seeding: {str(e)}")
                self.seeder = None
        else:
            self.seeder.stop()
            self.seeder = None
            self.seed_button.config(text="Start Seeding")
            self.seed_status.set("Not running")
            
    def seed_thread_func(self):
        try:
            self.seeder.start()
            while self.seeder.running:
                time.sleep(1)
        except Exception as e:
            messagebox.showerror("Error", f"Seeding error: {str(e)}")
            self.seeder = None
            self.seed_button.config(text="Start Seeding")
            self.seed_status.set("Not running")
            
    def toggle_download(self):
        if self.downloader is None:
            try:
                torrent_path = self.download_torrent_path.get()
                output_path = self.download_output_path.get()
                
                if not torrent_path or not output_path:
                    messagebox.showerror("Error", "Please select torrent file and set output file name")
                    return
                    
                # Ensure output path has an extension
                if not os.path.splitext(output_path)[1]:
                    messagebox.showerror("Error", "Output file must have an extension")
                    return
                    
                # Ensure download directory exists
                download_dir = os.path.dirname(output_path)
                if not os.path.exists(download_dir):
                    try:
                        os.makedirs(download_dir)
                    except Exception as e:
                        messagebox.showerror("Error", f"Failed to create download directory: {e}")
                        return
                    
                self.downloader = Downloader(torrent_path, output_path)
                
                # Check if there's a progress file and ask user if they want to resume
                progress_file = f"{output_path}.progress"
                if os.path.exists(progress_file) and self.resume_var.get():
                    if messagebox.askyesno("Resume Download", "A previous download was found. Would you like to resume?"):
                        self.download_status.set("Resuming download...")
                    else:
                        # Delete progress file to start fresh
                        try:
                            os.remove(progress_file)
                            self.download_status.set("Starting fresh download...")
                        except Exception as e:
                            messagebox.showerror("Error", f"Failed to delete progress file: {e}")
                            return
                else:
                    self.download_status.set("Starting download...")
                
                self.download_thread = threading.Thread(target=self.download_thread_func)
                self.download_thread.daemon = True
                self.download_thread.start()
                
                self.download_button.config(text="Stop Download")
                self.last_downloaded = 0
                self.last_time = time.time()
                
            except Exception as e:
                messagebox.showerror("Error", f"Failed to start download: {str(e)}")
                self.downloader = None
        else:
            self.downloader.stop()
            self.downloader = None
            self.download_button.config(text="Start Download")
            self.download_status.set("Not running")
            self.progress_var.set(0)
            self.speed_var.set("Speed: 0 B/s")
            
    def download_thread_func(self):
        try:
            self.downloader.start()
            while self.downloader.running:
                time.sleep(1)
                if self.downloader.num_pieces and self.downloader.downloaded_pieces:
                    progress = (self.downloader.downloaded_pieces / self.downloader.num_pieces) * 100
                    self.progress_var.set(progress)
                    
                    # Calculate download speed
                    current_time = time.time()
                    time_diff = current_time - self.last_time
                    if time_diff >= 1.0:  # Update speed every second
                        downloaded_diff = self.downloader.downloaded_pieces - self.last_downloaded
                        speed = (downloaded_diff * self.downloader.piece_length) / time_diff
                        self.speed_var.set(f"Speed: {self.format_speed(speed)}")
                        self.last_downloaded = self.downloader.downloaded_pieces
                        self.last_time = current_time
                        
        except Exception as e:
            messagebox.showerror("Error", f"Download error: {str(e)}")
            self.downloader = None
            self.download_button.config(text="Start Download")
            self.download_status.set("Not running")
            
    def format_speed(self, speed):
        """Format speed in bytes per second to human readable format."""
        for unit in ['B/s', 'KB/s', 'MB/s', 'GB/s']:
            if speed < 1024:
                return f"{speed:.2f} {unit}"
            speed /= 1024
        return f"{speed:.2f} TB/s"

def main():
    root = tk.Tk()
    app = TorrentGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main() 