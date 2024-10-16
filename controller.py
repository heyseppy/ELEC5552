import socket
import threading
import time
import tkinter as tk
from tkinter import scrolledtext, messagebox
from tkinter import ttk

# ESP32 Wi-Fi AP details
esp32_ip = '192.168.4.1'  # IP address for the ESP32
port = 80                 # Port for HTTP server on the ESP32

# Global variables
sock_lock = threading.Lock()

# Initialize Tkinter GUI components
root = tk.Tk()
root.title("Team01 Design Control Interface Rev2")
root.geometry("900x600")
root.resizable(False, False)

# Styling with ttk
style = ttk.Style()
style.theme_use('clam')
style.configure("TButton", font=("Arial", 12), padding=10)

# Frames for layout organization
top_frame = ttk.Frame(root)
top_frame.pack(pady=10)

middle_frame = ttk.Frame(root)
middle_frame.pack(pady=10)

bottom_frame = ttk.Frame(root)
bottom_frame.pack(pady=10, fill='both', expand=True)

status_frame = ttk.Frame(root)
status_frame.pack(side='bottom', fill='x')

# Log Display Area
log_area = scrolledtext.ScrolledText(bottom_frame, width=100, height=20, state='disabled', wrap='word')
log_area.pack(padx=10, pady=5)

# Append messages to the log area
def append_log(message):
    log_area.config(state='normal')
    log_area.insert(tk.END, f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {message}\n")
    log_area.see(tk.END)
    log_area.config(state='disabled')

# Send HTTP GET Request to ESP32
def send_command(parameter, value):
    command = f"GET /update?{parameter}={value} HTTP/1.1\r\nHost: {esp32_ip}\r\n\r\n"
    with sock_lock:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((esp32_ip, port))
            sock.send(command.encode())
            append_log(f"Sent command: {parameter}={value}")
            sock.close()
        except socket.error as e:
            append_log(f"Failed to send command {parameter}={value}. Socket error: {e}")
        except Exception as e:
            append_log(f"Unexpected error: {e}")

# GUI Functionality
def send_roll(val):
    send_command("roll", int(float(val)))

def send_pitch(val):
    send_command("pitch", int(float(val)))

def send_yaw(val):
    send_command("yaw", int(float(val)))

def send_throttle(val):
    send_command("throttle", int(float(val)))

def toggle_arm():
    global is_armed
    is_armed = not is_armed
    arm_button.config(text="Disarm" if is_armed else "Arm")
    send_command("arm", "1" if is_armed else "0")

def stop_motors():
    append_log("Stopping motors...")
    send_command("stop", "1")
    send_command("roll", "0")
    send_command("pitch", "0")
    send_command("yaw", "0")
    send_command("throttle", "0")

# GUI Layout with Tkinter Sliders and Buttons
def setup_gui():
    global roll_slider, pitch_slider, yaw_slider, throttle_slider, arm_button, is_armed

    # Control sliders
    roll_label = ttk.Label(middle_frame, text="Roll:")
    roll_label.grid(row=0, column=0)
    roll_slider = ttk.Scale(middle_frame, from_=-100, to=100, orient='horizontal', command=send_roll)
    roll_slider.grid(row=0, column=1, padx=10)

    pitch_label = ttk.Label(middle_frame, text="Pitch:")
    pitch_label.grid(row=1, column=0)
    pitch_slider = ttk.Scale(middle_frame, from_=-100, to=100, orient='horizontal', command=send_pitch)
    pitch_slider.grid(row=1, column=1, padx=10)

    yaw_label = ttk.Label(middle_frame, text="Yaw:")
    yaw_label.grid(row=2, column=0)
    yaw_slider = ttk.Scale(middle_frame, from_=-100, to=100, orient='horizontal', command=send_yaw)
    yaw_slider.grid(row=2, column=1, padx=10)

    throttle_label = ttk.Label(middle_frame, text="Throttle:")
    throttle_label.grid(row=3, column=0)
    throttle_slider = ttk.Scale(middle_frame, from_=0, to=100, orient='horizontal', command=send_throttle)
    throttle_slider.grid(row=3, column=1, padx=10)

    # Arm/Disarm button
    is_armed = False
    arm_button = ttk.Button(middle_frame, text="Arm", command=toggle_arm)
    arm_button.grid(row=4, column=0, columnspan=2, padx=20, pady=10, ipadx=10, ipady=10)

    # Stop Motors button
    stop_button = ttk.Button(middle_frame, text="Stop Motors", command=stop_motors)
    stop_button.grid(row=5, column=0, columnspan=2, padx=20, pady=10, ipadx=10, ipady=10)

# Set up the GUI and run the application
setup_gui()
root.mainloop()
