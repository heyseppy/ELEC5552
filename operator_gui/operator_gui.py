import socket
import threading
import time
import tkinter as tk
from tkinter import scrolledtext, messagebox
from tkinter import ttk
import cv2
from PIL import Image, ImageTk
import numpy as np
import logging

# Configuration Constants
SSID = "ESP32-Test-AP"
PASSWORD = "12345678"
ESP32_IP = '172.20.10.13'  # Replace with your ESP32's IP address
PORT = 12345               # TCP port for obstacle distance
STREAM_URL = 'http://172.20.10.12:81/stream'  # ESP32-CAM stream URL

# Configure Logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class NetworkClient:
    """Handles network communication with the ESP32."""

    def __init__(self, ip, port, logger, update_distance_callback, update_status_callback):
        self.ip = ip
        self.port = port
        self.logger = logger
        self.update_distance_callback = update_distance_callback
        self.update_status_callback = update_status_callback
        self.sock = None
        self.sock_file = None
        self.lock = threading.Lock()
        self.connected_event = threading.Event()
        self.stop_event = threading.Event()

    def connect(self):
        """Establish a connection to the ESP32 server."""
        while not self.stop_event.is_set():
            try:
                self.logger.info("Attempting to connect to the server...")
                self.update_status_callback("Reconnecting...")
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((self.ip, self.port))
                with self.lock:
                    self.sock = s
                    self.sock_file = s.makefile('r')
                self.logger.info("Connected to the server.")
                self.update_status_callback("Connected")
                self.connected_event.set()
                return
            except socket.error as e:
                self.logger.error(f"Connection failed: {e}. Retrying in 5 seconds...")
                self.update_status_callback("Disconnected")
                time.sleep(5)

    def receive_data(self):
        """Continuously receive obstacle distance data from the ESP32."""
        while not self.stop_event.is_set():
            if not self.connected_event.is_set():
                self.connect()

            try:
                with self.lock:
                    if self.sock_file:
                        obstacle_data = self.sock_file.readline().strip()
                        if obstacle_data:
                            self.logger.info(f"Received obstacle distance: {obstacle_data}")
                            self.update_distance_callback(obstacle_data)
            except socket.error as e:
                self.logger.error(f"Socket error: {e}")
                self.update_status_callback("Disconnected")
                self.connected_event.clear()
            except Exception as e:
                self.logger.error(f"Unexpected error: {e}")
                self.connected_event.clear()

            time.sleep(1)

    def send_command(self, command):
        """Send a command to the ESP32."""
        with self.lock:
            if self.sock:
                try:
                    self.sock.sendall((command + "\n").encode())
                    self.logger.info(f"Sent command: {command}")
                except socket.error as e:
                    self.logger.error(f"Failed to send command '{command}': {e}")
                    self.update_status_callback("Disconnected")
                    self.connected_event.clear()

    def start(self):
        """Start the network client threads."""
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread.start()

    def stop(self):
        """Stop the network client and close the socket."""
        self.stop_event.set()
        with self.lock:
            if self.sock_file:
                self.sock_file.close()
            if self.sock:
                self.sock.close()
                self.sock = None
        self.connected_event.clear()


class VideoStreamHandler:
    """Handles video streaming and line detection from the ESP32-CAM."""

    def __init__(self, stream_url, logger, update_angle_callback, update_feed_callback):
        self.stream_url = stream_url
        self.logger = logger
        self.update_angle_callback = update_angle_callback
        self.update_feed_callback = update_feed_callback
        self.stop_event = threading.Event()

    def detect_line_angle(self, frame):
        """Detect the line angle from the camera feed using OpenCV."""
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        edges = cv2.Canny(gray, 50, 150, apertureSize=3)
        lines = cv2.HoughLines(edges, 1, np.pi / 180, 150)

        if lines is not None:
            for rho, theta in lines[0]:
                a = np.cos(theta)
                b = np.sin(theta)
                x0 = a * rho
                y0 = b * rho
                x1 = int(x0 + 1000 * (-b))
                y1 = int(y0 + 1000 * (a))
                x2 = int(x0 - 1000 * (-b))
                y2 = int(y0 - 1000 * (a))
                angle = np.degrees(np.arctan2(y2 - y1, x2 - x1))
                return angle
        return None

    def stream_video(self):
        """Stream video from the ESP32-CAM and process each frame."""
        cap = cv2.VideoCapture(self.stream_url)
        if not cap.isOpened():
            self.logger.error("Unable to open the video stream.")
            self.update_feed_callback("Unable to open video stream.")
            return

        self.logger.info("Video stream started.")
        while not self.stop_event.is_set():
            ret, frame = cap.read()
            if ret:
                try:
                    angle = self.detect_line_angle(frame)
                    if angle is not None:
                        self.update_angle_callback(f"Line Angle: {angle:.2f}°")

                    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    img = Image.fromarray(frame_rgb)
                    imgtk = ImageTk.PhotoImage(image=img)
                    self.update_feed_callback(imgtk)
                except Exception as e:
                    self.logger.error(f"Error processing video frame: {e}")
            else:
                self.logger.error("Failed to read frame from video stream.")
                self.update_feed_callback("Failed to read frame.")
                break

            time.sleep(0.03)  # Approximately 30 FPS

        cap.release()

    def start(self):
        """Start the video streaming thread."""
        self.video_thread = threading.Thread(target=self.stream_video, daemon=True)
        self.video_thread.start()

    def stop(self):
        """Stop the video streaming."""
        self.stop_event.set()


class Simulator:
    """Simulates drone states for testing purposes."""

    def __init__(self, logger, update_state_callback):
        self.logger = logger
        self.update_state_callback = update_state_callback
        self.states = ["Lifting", "Hovering", "Following Line", "Stopping because of obstacle"]
        self.current_index = 0
        self.simulation_thread = None
        self.stop_event = threading.Event()
        self.simulation_running = False

    def run_simulation(self):
        """Cycle through predefined drone states."""
        while not self.stop_event.is_set():
            state = self.states[self.current_index]
            self.logger.info(f"Simulation State: {state}")
            self.update_state_callback(state)
            self.current_index = (self.current_index + 1) % len(self.states)
            time.sleep(1)

    def start(self):
        """Start the simulation."""
        if not self.simulation_running:
            self.simulation_running = True
            self.stop_event.clear()
            self.simulation_thread = threading.Thread(target=self.run_simulation, daemon=True)
            self.simulation_thread.start()
            self.logger.info("Drone simulation started.")

    def stop(self):
        """Stop the simulation."""
        if self.simulation_running:
            self.stop_event.set()
            if self.simulation_thread.is_alive():
                self.simulation_thread.join(timeout=2)
            self.simulation_running = False
            self.logger.info("Drone simulation stopped.")


class DroneControlApp:
    """Main application class for the Drone Control Interface."""

    def __init__(self, root):
        self.root = root
        self.root.title("Team01 Design Control Interface Rev2")
        self.root.geometry("900x800")
        self.root.resizable(False, False)

        # Initialize GUI Components
        self.setup_styles()
        self.create_frames()
        self.create_widgets()

        # Initialize Logging Area
        self.setup_logging()

        # Initialize Network Client
        self.network_client = NetworkClient(
            ip=ESP32_IP,
            port=PORT,
            logger=self.logger,
            update_distance_callback=self.update_distance_label,
            update_status_callback=self.update_connection_status
        )

        # Initialize Video Stream Handler
        self.video_stream_handler = VideoStreamHandler(
            stream_url=STREAM_URL,
            logger=self.logger,
            update_angle_callback=self.update_angle_label,
            update_feed_callback=self.update_camera_feed
        )

        # Initialize Simulator
        self.simulator = Simulator(
            logger=self.logger,
            update_state_callback=self.update_state_label
        )

        # Start Network and Video Threads
        self.network_client.start()
        self.video_stream_handler.start()

        # Handle Window Close Event
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    def setup_styles(self):
        """Configure ttk styles."""
        style = ttk.Style()
        style.theme_use('clam')
        style.configure("TButton", font=("Arial", 12), padding=10)
        style.configure("Status.TLabel", font=("Arial", 10), foreground="red")
        style.configure("Red.TButton", background="red", foreground="white")
        style.configure("Yellow.TButton", background="yellow", foreground="black")
        style.configure("Green.TButton", background="green", foreground="white")
        style.configure("Blue.TButton", background="blue", foreground="white")

    def create_frames(self):
        """Create and organize frames for layout."""
        self.top_frame = ttk.Frame(self.root)
        self.top_frame.pack(pady=10)

        self.middle_frame = ttk.Frame(self.root)
        self.middle_frame.pack(pady=10)

        self.bottom_frame = ttk.Frame(self.root)
        self.bottom_frame.pack(pady=10, fill='both', expand=True)

        self.status_frame = ttk.Frame(self.root)
        self.status_frame.pack(side='bottom', fill='x')

    def create_widgets(self):
        """Create and place all GUI widgets."""
        # Connection Status Indicator
        self.connection_status = ttk.Label(self.status_frame, text="Disconnected", style="Status.TLabel")
        self.connection_status.pack(side='left', padx=10)

        # Log Display Area
        log_label = ttk.Label(self.bottom_frame, text="Log:")
        log_label.pack(anchor='w', padx=10)

        self.log_area = scrolledtext.ScrolledText(self.bottom_frame, width=100, height=10, state='disabled', wrap='word')
        self.log_area.pack(padx=10, pady=5)

        # Message Display Label
        self.message_label = ttk.Label(self.top_frame, text="", font=("Arial", 14))
        self.message_label.pack(pady=5)

        # Drone State Display Label
        self.state_label = ttk.Label(self.top_frame, text="State: N/A", font=("Arial", 16, "bold"), foreground="purple")
        self.state_label.pack(pady=10)

        # Line Angle Display Label
        self.angle_label = ttk.Label(self.top_frame, text="Line Angle: N/A", font=("Arial", 20, "bold"), foreground="blue")
        self.angle_label.pack(pady=10)

        # Obstacle Distance Display Label
        self.distance_label = ttk.Label(self.top_frame, text="Obstacle Distance: N/A", font=("Arial", 20, "bold"), foreground="red")
        self.distance_label.pack(pady=10)

        # Video Frame to display the camera feed
        self.video_frame = ttk.Frame(self.root)
        self.video_frame.pack(pady=10, padx=10)

        self.camera_feed_label = tk.Label(self.video_frame)
        self.camera_feed_label.pack()

        # Control Buttons
        self.create_control_buttons()

    def create_control_buttons(self):
        """Create control buttons and simulation button."""
        # Emergency Stop Button
        emergency_button = ttk.Button(
            self.middle_frame,
            text="Emergency Stop",
            command=self.emergency_stop,
            style="Red.TButton"
        )
        emergency_button.grid(row=0, column=0, padx=20, pady=10, ipadx=10, ipady=10)

        # Hover Mode Button
        hover_button = ttk.Button(
            self.middle_frame,
            text="Hover Mode",
            command=lambda: self.send_command("HOVER"),
            style="Yellow.TButton"
        )
        hover_button.grid(row=0, column=1, padx=20, pady=10, ipadx=10, ipady=10)

        # Manual Mode Button
        manual_button = ttk.Button(
            self.middle_frame,
            text="Manual Mode",
            command=lambda: self.send_command("MANUAL"),
            style="Green.TButton"
        )
        manual_button.grid(row=0, column=2, padx=20, pady=10, ipadx=10, ipady=10)

        # Autonomous Control Button
        autonomous_button = ttk.Button(
            self.middle_frame,
            text="Autonomous Control",
            command=lambda: self.send_command("AUTO"),
            style="Blue.TButton"
        )
        autonomous_button.grid(row=0, column=3, padx=20, pady=10, ipadx=10, ipady=10)

        # Simulation Button
        self.simulate_button = ttk.Button(
            self.middle_frame,
            text="Start Simulation",
            command=self.toggle_simulation,
            style="Blue.TButton"
        )
        self.simulate_button.grid(row=1, column=0, columnspan=4, padx=20, pady=10, ipadx=10, ipady=10)

    def setup_logging(self):
        """Configure the logging area."""
        self.logger = logging.getLogger("DroneControlApp")
        self.logger.setLevel(logging.INFO)

        # Create a handler that writes log messages to the GUI
        gui_handler = GUIHandler(self.log_area)
        gui_handler.setLevel(logging.INFO)

        # Add the handler to the logger
        self.logger.addHandler(gui_handler)

    def update_connection_status(self, status):
        """Update the connection status label."""
        self.connection_status.config(text=status)
        if status == "Connected":
            self.connection_status.config(foreground="green")
        elif status == "Disconnected":
            self.connection_status.config(foreground="red")
        elif status == "Reconnecting...":
            self.connection_status.config(foreground="orange")

    def update_distance_label(self, distance):
        """Update the obstacle distance label."""
        self.distance_label.config(text=f"Obstacle Distance: {distance}")

    def update_angle_label(self, angle_text):
        """Update the line angle label."""
        self.angle_label.config(text=angle_text)

    def update_camera_feed(self, imgtk):
        """Update the camera feed in the GUI."""
        if isinstance(imgtk, str):
            self.camera_feed_label.config(text=imgtk)
        else:
            self.camera_feed_label.imgtk = imgtk
            self.camera_feed_label.config(image=imgtk)

    def update_state_label(self, state):
        """Update the drone state label."""
        self.state_label.config(text=f"State: {state}")

    def send_command(self, command):
        """Send a command to the ESP32."""
        self.network_client.send_command(command)
        self.message_label.config(text=f"Command Sent: {command}")

    def emergency_stop(self):
        """Handle Emergency Stop action."""
        self.logger.info("Emergency Stop triggered.")
        self.send_command("STOP")
        self.message_label.config(text="Emergency Stop")

    def toggle_simulation(self):
        """Toggle the drone simulation on or off."""
        if not self.simulator.simulation_running:
            self.simulator.start()
            self.simulate_button.config(text="Stop Simulation")
        else:
            self.simulator.stop()
            self.simulate_button.config(text="Start Simulation")

    def on_closing(self):
        """Handle the window closing event."""
        if messagebox.askokcancel("Quit", "Do you want to quit the application?"):
            self.logger.info("Shutting down application...")
            self.network_client.stop()
            self.video_stream_handler.stop()
            self.simulator.stop()
            self.root.destroy()


class GUIHandler(logging.Handler):
    """Custom logging handler to display logs in the GUI."""

    def __init__(self, text_widget):
        super().__init__()
        self.text_widget = text_widget

    def emit(self, record):
        msg = self.format(record)
        self.text_widget.configure(state='normal')
        self.text_widget.insert(tk.END, msg + "\n")
        self.text_widget.configure(state='disabled')
        self.text_widget.see(tk.END)


def main():
    """Main function to start the Drone Control Application."""
    root = tk.Tk()
    app = DroneControlApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
