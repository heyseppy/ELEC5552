import socket
import threading
import time
import tkinter as tk
from tkinter import scrolledtext, messagebox
from tkinter import ttk

# ESP32 Wi-Fi AP details
ssid = "ESP32-Test-AP"
password = "12345678"
esp32_ip = '192.168.4.1'  # Replace with your ESP32's IP address if different
port = 80                 # TCP port to connect to
heartbeat_command = "HEARTBEAT"

# Global variables
stop_heartbeat = False
sock = None
sock_file = None  # File-like object for reading from socket
sock_lock = threading.Lock()  # Lock for socket access

# Initialize Tkinter GUI components
root = tk.Tk()
root.title("Team01 Design Control Interface Rev2")
root.geometry("900x600")  # Increased size for better layout
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

log_area = scrolledtext.ScrolledText(bottom_frame, width=100, height=20, state='disabled', wrap='word')
log_area.pack(padx=10, pady=5)

# Message Display Label
message_label = ttk.Label(top_frame, text="", font=("Arial", 14))
message_label.pack(pady=5)

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

def send_command(command, expect_response=True):
    """Send a command to the ESP32 over the persistent TCP connection."""
    global sock, sock_file
    try:
        # Ensure the connection is alive
        if sock is None:
            connect_to_server()
        with sock_lock:
            sock.sendall((command + '\n').encode())
            append_log(f"Sent command: {command}")
            if expect_response:
                response_line = sock_file.readline()
                if not response_line:
                    # Connection closed
                    append_log("Connection closed by the server.")
                    update_connection_status("Disconnected")
                    sock_file.close()
                    sock.close()
                    sock = None
                else:
                    response = response_line.strip()
                    append_log(f"Received response: {response}")
                    # Update the GUI with the response
                    root.after(0, lambda: display_message(response))
    except socket.error as e:
        append_log(f"Socket error: {e}")
        update_connection_status("Disconnected")
        # Handle disconnection and attempt to reconnect
        with sock_lock:
            if sock_file:
                sock_file.close()
                sock_file = None
            if sock:
                sock.close()
                sock = None
        connect_to_server()
    except Exception as e:
        append_log(f"Unexpected error: {e}")

def send_heartbeat():
    """Continuously send heartbeat signals every second."""
    global stop_heartbeat
    while not stop_heartbeat:
        try:
            send_command(heartbeat_command, expect_response=False)  # Send the HEARTBEAT command
            time.sleep(1)  # Send heartbeat every 1 second
        except Exception as e:
            append_log(f"Error in heartbeat thread: {e}")
            break

# GUI Functions
def animate_button(button, original_color):
    # Change button appearance to indicate it was clicked
    button.state(['pressed'])
    button.config(style='Pressed.TButton')
    root.after(200, lambda: reset_button_style(button, original_color))

def reset_button_style(button, color):
    button.state(['!pressed'])
    button.config(style=f"{color}.TButton")

def display_message(message):
    # Update the text of the message label
    message_label.config(text=message)

def emergency():
    append_log("Emergency Stop triggered.")
    display_message("Emergency Stop")
    animate_button(emergency_button, "Red")
    send_command("STOP")  # Assuming "STOP" corresponds to Emergency Stop

def hover():
    append_log("Hover Mode activated.")
    display_message("Hover Mode")
    animate_button(hover_button, "Yellow")
    send_command("HOVER")

def manual():
    append_log("Manual Mode activated.")
    display_message("Manual Mode")
    animate_button(manual_button, "Green")
    send_command("MANUAL")

def autonomous():
    append_log("Autonomous Control activated.")
    display_message("Autonomous Control")
    animate_button(autonomous_button, "Blue")
    send_command("AUTO")

def send_custom_command():
    command = custom_command_entry.get().strip()
    if command:
        append_log(f"Custom command sent: {command}")
        display_message(f"Custom Command: {command}")
        send_command(command)
        custom_command_entry.delete(0, tk.END)
    else:
        messagebox.showwarning("Input Error", "Please enter a command to send.")

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

# Graceful Exit Handling
def on_closing():
    """Handle the window closing event."""
    global stop_heartbeat
    if messagebox.askokcancel("Quit", "Do you want to quit the application?"):
        append_log("Shutting down application...")
        stop_heartbeat = True  # Stop the heartbeat thread
        if heartbeat_thread.is_alive():
            heartbeat_thread.join()  # Wait for heartbeat thread to finish
        with sock_lock:
            if sock_file:
                sock_file.close()
                sock_file = None
            if sock:
                sock.close()
                sock = None
        root.destroy()

# Main Function
def main():
    global heartbeat_thread
    # Establish initial connection to the server
    connection_thread = threading.Thread(target=connect_to_server, daemon=True)
    connection_thread.start()

    # Start sending heartbeats in a separate thread
    heartbeat_thread = threading.Thread(target=send_heartbeat, daemon=True)
    heartbeat_thread.start()

    # Setup the GUI
    setup_gui()

    # Handle window close event
    root.protocol("WM_DELETE_WINDOW", on_closing)

    # Start the Tkinter event loop
    root.mainloop()

if __name__ == "__main__":
    main()

