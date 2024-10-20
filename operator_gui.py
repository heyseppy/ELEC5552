import tkinter as tk
from tkinter import scrolledtext
import cv2
from PIL import Image, ImageTk
import threading
import requests
import time

# IP address of the ESP32-CAM
CAMERA_IP = "http://172.20.10.12:81/stream"  # Replace with your ESP32-CAM stream IP
COMMAND_URL = 'http://172.xx.xx.xx/'      # Replace with the ESP32-CAM command URL (if needed)

class ESP32CamController:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32-CAM Controller")
        
        # Camera feed label
        self.camera_feed_label = tk.Label(root)
        self.camera_feed_label.grid(row=0, column=0, columnspan=2, padx=10, pady=10)
        
        # Event log
        self.event_log = scrolledtext.ScrolledText(root, width=40, height=10)
        self.event_log.grid(row=0, column=2, padx=10, pady=10)
        
        # Buttons for actions
        self.btn_start = tk.Button(root, text="Start", command=self.start_command)
        self.btn_stop = tk.Button(root, text="Stop", command=self.stop_command)
        self.btn_hover = tk.Button(root, text="Hover", command=self.hover_command)
        self.btn_manual = tk.Button(root, text="Manual", command=self.manual_command)

        self.btn_start.grid(row=1, column=0, padx=5, pady=5)
        self.btn_stop.grid(row=1, column=1, padx=5, pady=5)
        self.btn_hover.grid(row=2, column=0, padx=5, pady=5)
        self.btn_manual.grid(row=2, column=1, padx=5, pady=5)

        # Start the thread for capturing the camera feed
        self.capture_thread = threading.Thread(target=self.update_camera_feed)
        self.capture_thread.daemon = True
        self.capture_thread.start()

    def log_event(self, event_message):
        """Logs events to the event log."""
        self.event_log.insert(tk.END, f"{time.strftime('%H:%M:%S')}: {event_message}\n")
        self.event_log.see(tk.END)  # Scroll to the bottom

    def send_command(self, command):
        """Sends a command to the ESP32-CAM."""
        try:
            requests.get(COMMAND_URL + command)
            self.log_event(f"Sent command: {command}")
        except Exception as e:
            self.log_event(f"Failed to send command: {command} - {e}")

    def start_command(self):
        """Starts the system (e.g., for autonomous mode)."""
        self.send_command('start')

    def stop_command(self):
        """Stops the system (e.g., emergency stop)."""
        self.send_command('stop')

    def hover_command(self):
        """Hover command."""
        self.send_command('hover')

    def manual_command(self):
        """Switches to manual mode."""
        self.send_command('manual')

    def update_camera_feed(self):
        """Continuously captures frames from the ESP32-CAM and updates the GUI."""
        cap = cv2.VideoCapture(CAMERA_IP)
        while True:
            ret, frame = cap.read()
            if ret:
                # Convert the image to a format Tkinter can display
                frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                img = Image.fromarray(frame_rgb)
                imgtk = ImageTk.PhotoImage(image=img)
                self.camera_feed_label.imgtk = imgtk
                self.camera_feed_label.config(image=imgtk)
            time.sleep(0.1)  # Adjust the delay as needed

        cap.release()

if __name__ == "__main__":
    root = tk.Tk()
    app = ESP32CamController(root)
    root.mainloop()
