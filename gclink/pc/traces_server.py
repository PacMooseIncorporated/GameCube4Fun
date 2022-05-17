import socket
import sys

HOST = "192.168.1.53"
PORT = 10000  # Port to listen on (non-privileged ports are > 1023)
TIMEOUT = 60

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the port
server_address = (HOST, PORT)

print('Gamecube BBA Trace Server by Shazz/TRSi')
print('Starting up on {} port {}'.format(*server_address))
sock.bind(server_address)
sock.settimeout(TIMEOUT)

print('\nWaiting to receive message...')
try:
    while True:
        data, address = sock.recvfrom(4096)
        print(f'[{address[0]}][{len(data)}] {data.decode("ascii")}')

except KeyboardInterrupt:
    print("Exiting!")
    sys.exit()
except socket.timeout:
    print("Timeout, exiting!")
    sys.exit()
