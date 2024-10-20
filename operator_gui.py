import socket
import threading
import time
import tkinter as tk
from tkinter import scrolledtext, messagebox
from tkinter import ttk
import cv2
from PIL import Image, ImageTk
import requests
import numpy as np

# ESP32 Wi-Fi AP details
ssid = "ESP32-Test-AP"
password = "12345678"
esp32_ip = '172.20.10.13'  # Replace with your ESP32's IP address
port = 12345               # TCP port for obstacle distance
heartbeat_command = "HEARTBEAT"
stream_url = 'http://172.20.10.12:81/stream'  # ESP32-CAM stream URL

# Global variables
stop_heartbeat = False
sock = None
sock_file = None  # File-like object for reading from socket
sock_lock = threading.Lock()  # Lock for socket access

# Initialize Tkinter GUI components
root = tk.Tk()
root.title("Team01 Design Control Interface Rev2")
root.geometry("900x800")  # Increased size for better layout
root.resizable(False, False)  # Fixed window size

# Styling with ttk
style = ttk.Style()
style.theme_use('clam')  # You can choose other themes like 'default', 'alt', 'classic', etc.
style.configure("TButton", font=("Arial", 12), padding=10)
style.configure("Status.TLabel", font=("Arial", 10), foreground="green")

# Frames for better layout organization
top_frame = ttk.Frame(root)
top_frame.pack(pady=10)

middle_frame = ttk.Frame(root)
middle_frame.pack(pady=10)

bottom_frame = ttk.Frame(root)
bottom_frame.pack(pady=10, fill='both', expand=True)

status_frame = ttk.Frame(root)
status_frame.pack(side='bottom', fill='x')

# Connection Status Indicator
connection_status = ttk.Label(status_frame, text="Disconnected", style="Status.TLabel")
connection_status.pack(side='left', padx=10)

# Log Display Area
log_label = ttk.Label(bottom_frame, text="Log:")
log_label.pack(anchor='w', padx=10)

log_area = scrolledtext.ScrolledText(bottom_frame, width=100, height=10, state='disabled', wrap='word')
log_area.pack(padx=10, pady=5)

# Message Display Label
message_label = ttk.Label(top_frame, text="", font=("Arial", 14))
message_label.pack(pady=5)

# Line Angle Display Label
angle_label = ttk.Label(top_frame, text="Line Angle: N/A", font=("Arial", 20, "bold"), foreground="blue")
angle_label.pack(pady=10)

# Obstacle Distance Display Label
distance_label = ttk.Label(top_frame, text="", font=("Arial", 20, "bold"), foreground="red")
distance_label.pack(pady=10)

# Video Frame to display the camera feed under buttons
video_frame = ttk.Frame(root)
video_frame.pack(pady=10, padx=10)

camera_feed_label = tk.Label(video_frame)
camera_feed_label.pack()

# Function to update the connection status
def update_connection_status(status):
    connection_status.config(text=status)
    if status == "Connected":
        connection_status.config(foreground="green")
    elif status == "Disconnected":
        connection_status.config(foreground="red")
    elif status == "Reconnecting...":
        connection_status.config(foreground="orange")

# Function to append messages to the log area
def append_log(message):
    log_area.config(state='normal')
    log_area.insert(tk.END, f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {message}\n")
    log_area.see(tk.END)
    log_area.config(state='disabled')

# Socket Connection Functions
def connect_to_server():
    """Establish a persistent connection to the ESP32 server."""
    global sock, sock_file
    while True:
        try:
            append_log("Attempting to connect to the server...")
            update_connection_status("Reconnecting...")
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((esp32_ip, port))
            with sock_lock:
                sock = s
                sock_file = s.makefile('r')  # Create a file-like object for reading
            append_log("Connected to the server.")
            update_connection_status("Connected")
            break
        except socket.error as e:
            append_log(f"Connection failed: {e}. Retrying in 5 seconds...")
            update_connection_status("Disconnected")
            time.sleep(5)

# Function to handle receiving obstacle distance from the socket
def receive_obstacle_distance():
    """Continuously receive obstacle distance data from the ESP32."""
    global sock, sock_file
    while True:
        try:
            # Ensure the connection is alive
            if sock is None:
                connect_to_server()
            with sock_lock:
                while True:
                    if sock_file:
                        obstacle_data = sock_file.readline().strip()
                        if obstacle_data:
                            append_log(f"Received obstacle distance: {obstacle_data}")
                            root.after(0, lambda: distance_label.config(text=f"{obstacle_data}"))
        except socket.error as e:
            append_log(f"Socket error: {e}")
            update_connection_status("Disconnected")
            connect_to_server()
        except Exception as e:
            append_log(f"Unexpected error: {e}")
        time.sleep(1)

# Function to detect the line angle using OpenCV
def detect_line_angle(frame):
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

# Function to display video feed from ESP32-CAM and detect line
def update_camera_feed():
    cap = cv2.VideoCapture(stream_url)
    if not cap.isOpened():
        append_log("Unable to open the video stream.")
        root.after(0, lambda: camera_feed_label.config(text="Unable to open video stream."))
        return
    append_log("Video stream started.")
    while True:
        ret, frame = cap.read()
        if ret:
            try:
                frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                angle = detect_line_angle(frame)
                if angle is not None:
                    root.after(0, lambda: angle_label.config(text=f"Line Angle: {angle:.2f}°"))
                img = Image.fromarray(frame_rgb)
                imgtk = ImageTk.PhotoImage(image=img)
                camera_feed_label.imgtk = imgtk
                camera_feed_label.config(image=imgtk)
            except Exception as e:
                append_log(f"Error processing video frame: {e}")
        else:
            append_log("Failed to read frame from video stream.")
            root.after(0, lambda: camera_feed_label.config(text="Failed to read frame."))
            break
        time.sleep(0.03)  # Approximately 30 FPS
    cap.release()

# Graceful Exit Handling
def on_closing():
    """Handle the window closing event."""
    global stop_heartbeat
    if messagebox.askokcancel("Quit", "Do you want to quit the application?"):
        append_log("Shutting down application...")
        stop_heartbeat = True  # Stop the heartbeat thread
        if heartbeat_thread.is_alive():
            heartbeat_thread.join(timeout=2)  # Wait for heartbeat thread to finish
        if angle_thread.is_alive():
            angle_thread.join(timeout=2)  # Wait for angle thread to finish
        if distance_thread.is_alive():
            distance_thread.join(timeout=2)  # Wait for distance thread to finish
        with sock_lock:
            if sock_file:
                sock_file.close()
                sock_file = None
            if sock:
                sock.close()
                sock = None
        root.destroy()

# Functions for each button press action
def emergency():
    append_log("Emergency Stop triggered.")
    send_command("STOP")
    message_label.config(text="Emergency Stop")

def hover():
    append_log("Hover Mode activated.")
    send_command("HOVER")
    message_label.config(text="Hover Mode")

def manual():
    append_log("Manual Mode activated.")
    send_command("MANUAL")
    message_label.config(text="Manual Mode")

def autonomous():
    append_log("Autonomous Mode activated.")
    send_command("AUTO")
    message_label.config(text="Autonomous Mode")

def send_command(command):
    """Send a command to the ESP32."""
    append_log(f"Sending command: {command}")
    # Handle sending command via socket (or other communication method)

def setup_gui():
    global emergency_button, hover_button, manual_button, autonomous_button, custom_command_entry

    # Define custom styles for buttons
    style.configure("Red.TButton", background="red", foreground="white")
    style.configure("Yellow.TButton", background="yellow", foreground="black")
    style.configure("Green.TButton", background="green", foreground="white")
    style.configure("Blue.TButton", background="blue", foreground="white")
    style.configure("Pressed.TButton", background="lightgrey", foreground="black")

    # Create Buttons
    emergency_button = ttk.Button(middle_frame, text="Emergency Stop", command=emergency, style="Red.TButton")
    emergency_button.grid(row=0, column=0, padx=20, pady=10, ipadx=10, ipady=10)

    hover_button = ttk.Button(middle_frame, text="Hover Mode", command=hover, style="Yellow.TButton")
    hover_button.grid(row=0, column=1, padx=20, pady=10, ipadx=10, ipady=10)

    manual_button = ttk.Button(middle_frame, text="Manual Mode", command=manual, style="Green.TButton")
    manual_button.grid(row=0, column=2, padx=20, pady=10, ipadx=10, ipady=10)

    autonomous_button = ttk.Button(middle_frame, text="Autonomous Control", command=autonomous, style="Blue.TButton")
    autonomous_button.grid(row=0, column=3, padx=20, pady=10, ipadx=10, ipady=10)

    # Custom Command Section
    custom_command_label = ttk.Label(top_frame, text="Send Custom Command:", font=("Arial", 12))
    custom_command_label.pack(pady=5)

    custom_command_frame = ttk.Frame(top_frame)
    custom_command_frame.pack(pady=5)

    custom_command_entry = ttk.Entry(custom_command_frame, width=50, font=("Arial", 12))
    custom_command_entry.pack(side='left', padx=5)

    send_custom_button = ttk.Button(custom_command_frame, text="Send", command=send_custom_command)
    send_custom_button.pack(side='left', padx=5)

def send_custom_command():
    command = custom_command_entry.get().strip()
    if command:
        append_log(f"Custom command sent: {command}")
        send_command(command)
        custom_command_entry.delete(0, tk.END)
    else:
        messagebox.showwarning("Input Error", "Please enter a command to send.")

# Main Function
def main():
    global heartbeat_thread, angle_thread, distance_thread
    # Establish initial connection to the server
    connection_thread = threading.Thread(target=connect_to_server, daemon=True)
    connection_thread.start()

    # Start receiving obstacle distance in a separate thread
    distance_thread = threading.Thread(target=receive_obstacle_distance, daemon=True)
    distance_thread.start()

    # Start the camera feed thread with line detection
    camera_thread = threading.Thread(target=update_camera_feed, daemon=True)
    camera_thread.start()

    # Setup the GUI
    setup_gui()

    # Handle window close event
    root.protocol("WM_DELETE_WINDOW", on_closing)

    # Start the Tkinter event loop
    root.mainloop()

if __name__ == "__main__":
    main()
