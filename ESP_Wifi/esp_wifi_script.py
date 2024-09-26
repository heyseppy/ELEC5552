import socket
import threading
import time

# ESP32 Wi-Fi AP details
ssid = "ESP32-Test-AP"
password = "12345678"
esp32_ip = '192.168.4.1'  # Replace with ESP32's IP address if different
port = 80                 # TCP port to connect to
heartbeat_command = "HEARTBEAT"

# Global variables
stop_heartbeat = False
sock = None
sock_file = None  # Add a global variable for the file-like object
sock_lock = threading.Lock()  # Lock for socket access

def connect_to_server():
    """Establish a persistent connection to the ESP32 server."""
    global sock, sock_file
    while True:
        try:
            print("Connecting to the server...")
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((esp32_ip, port))
            with sock_lock:
                sock = s
                sock_file = s.makefile('r')  # Create a file-like object for reading
            print("Connected to the server.")
            break
        except socket.error as e:
            print(f"Connection failed: {e}. Retrying in 5 seconds...")
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
            if expect_response:
                response_line = sock_file.readline()
                if not response_line:
                    # Connection closed
                    print("Connection closed by the server.")
                    sock_file.close()
                    sock.close()
                    sock = None
                else:
                    print(f"Response: {response_line.strip()}")
    except socket.error as e:
        print(f"Error: {e}")
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
        print(f"Unexpected error: {e}")

def send_heartbeat():
    """Continuously send heartbeat signals every second."""
    global stop_heartbeat
    while not stop_heartbeat:
        try:
            send_command(heartbeat_command, expect_response=False)  # Send the HEARTBEAT command
            time.sleep(1)  # Send heartbeat every 1 second
        except Exception as e:
            print(f"Error in heartbeat thread: {e}")
            break

def main():
    global stop_heartbeat, sock, sock_file
    # Establish initial connection to the server
    connect_to_server()

    # Start sending heartbeats in a separate thread
    heartbeat_thread = threading.Thread(target=send_heartbeat)
    heartbeat_thread.daemon = True  # Daemonize the thread so it exits when the program exits
    heartbeat_thread.start()

    print("Available commands: MANUAL, STOP, AUTO, HOVER")

    # Allow the user to send manual commands
    try:
        while True:
            command = input("Enter a command (or 'exit' to quit): ").upper()
            if command == "EXIT":
                print("Exiting...")
                stop_heartbeat = True  # Stop the heartbeat thread
                heartbeat_thread.join()  # Wait for heartbeat thread to finish
                with sock_lock:
                    if sock_file:
                        sock_file.close()
                        sock_file = None
                    if sock:
                        sock.close()
                        sock = None
                break
            elif command in ["MANUAL", "STOP", "AUTO", "HOVER"]:
                send_command(command)  # Send the manual command
            else:
                print("Invalid command! Please try again.")
    except KeyboardInterrupt:
        print("\nExiting...")
        stop_heartbeat = True
        heartbeat_thread.join()
        with sock_lock:
            if sock_file:
                sock_file.close()
                sock_file = None
            if sock:
                sock.close()
                sock = None

if __name__ == "__main__":
    main()

