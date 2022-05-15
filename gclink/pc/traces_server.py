import socket
import sys

HOST = "192.168.1.53"
PORT = 10000  # Port to listen on (non-privileged ports are > 1023)

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the port
server_address = (HOST, PORT)
print('Starting up on {} port {}'.format(*server_address))
sock.bind(server_address)

print('\nWaiting to receive message')
while True:
    data, address = sock.recvfrom(4096)

    print(f'received {len(data)} bytes from {address}')
    print(f'Message: "{data}"')
