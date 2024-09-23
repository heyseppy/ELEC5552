import socket
import subprocess
import time

# WiFi details of the ESP32 AP
ssid = "ESP32-LR-Access-Point"
password = "12345678"
esp32_ip = '192.168.4.1'  # Default IP of ESP32 in AP mode
port = 80                 # TCP port

# Commands to be sent
commands = ["MANUAL MODE", "STOP", "AUTO", "HOVER"]

def connect_to_wifi(ssid, password):
    """Connect to the ESP32 Wi-Fi access point."""
    try:
        # Command to connect to the Wi-Fi (works on Linux/macOS; adjust for Windows)
        print(f"Connecting to Wi-Fi SSID: {ssid}")
        subprocess.run(["nmcli", "device", "wifi", "connect", ssid, "password", password], check=True)
        print("Connected to Wi-Fi.")
    except subprocess.CalledProcessError as e:
        print(f"Failed to connect to Wi-Fi: {e}")
        return False

    # Wait a few seconds for the connection to establish
    time.sleep(5)
    return True

def send_command(command):
    """Send a command to the ESP32 over TCP."""
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            # Connect to the ESP32
            sock.connect((esp32_ip, port))
            print(f"Connected to ESP32 at {esp32_ip}:{port}")
            
            # Send the command
            sock.sendall((command + '\n').encode())
            print(f"Sent command: {command}")
            
            # Receive the response
            response = sock.recv(1024).decode()
            print(f"Received response: {response}")
    
    except socket.error as e:
        print(f"Error connecting or sending data: {e}")

def main():
    """Main function to connect and send commands to the ESP32."""
    # Connect to the ESP32 access point
    if not connect_to_wifi(ssid, password):
        print("Unable to connect to ESP32 access point.")
        return
    
    print("Available commands: MANUAL MODE, STOP, AUTO, HOVER")
    
    while True:
        # Ask the user for input
        command = input("Enter a command to send to the ESP32 (or 'exit' to quit): ").upper()
        
        # Exit the loop if the user types 'exit'
        if command == "EXIT":
            print("Exiting...")
            break
        
        # Check if the command is valid
        if command in commands:
            send_command(command)
        else:
            print("Invalid command! Please try again.")

if __name__ == "__main__":
    main()
