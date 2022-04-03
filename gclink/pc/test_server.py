# echo-server.py

import socket

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 2323  # Port to listen on (non-privileged ports are > 1023)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()

    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")
        while True:
            data = conn.recv(1024)
            if not data:
                break

            res = data.decode("ascii").split(' ')
            print(f"received {res}")

            if "EXECELF" in res[0]:
                print(f"ready to received file {res[1]} of size {res[2]}")
                conn.sendall(b"OK")
                data = conn.recv(int(res[2]))

                print(f"received file content: {data!r}")

                conn.sendall(b"ELF executed")

            elif "EXECDOL" in res[0]:
                print(f"ready to received file {res[1]} of size {res[2]}")
                conn.sendall(b"OK")
                data = conn.recv(int(res[2]))

                print(f"received file content: {data!r}")

                conn.sendall(b"DOL executed")

            elif "RESET" in res:
                conn.sendall(b"RESET!")
            else:
                conn.sendall(b"THANKS!")
