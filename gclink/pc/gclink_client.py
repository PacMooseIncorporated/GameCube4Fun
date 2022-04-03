import socket
import ipaddress
import argparse
from pathlib import Path

def check_port(value):
    ivalue = int(value)
    if ivalue not in range(1, 65536):
        raise argparse.ArgumentTypeError(f"{ivalue} is an invalid port value")
    return ivalue

def check_host(value):

    if all([c.isdigit() or c == '.' for c in value]):
        try:
            ip = ipaddress.ip_address(value)
        except ValueError:
            raise argparse.ArgumentTypeError(f"{value} is an invalid ip address value")
    else:
        print(f"hostname supplied: {value}")

    return value


if __name__ == '__main__':

    # Instantiate the parser
    parser = argparse.ArgumentParser(description='GCLink client by Shazz/TRSi')

    # general arguments
    parser.add_argument('--host', help='GCLink server hostname or ip', required=True, type=check_host)
    parser.add_argument('--port', default=2323, help='GCLink server port, default 2323', required=False, type=check_port)

    # commands sub parser
    subparsers = parser.add_subparsers(dest='command', help='Specific command help')

    # DOL exec command
    execdol = subparsers.add_parser('execdol', help='Execute DOL file')
    execdol.add_argument('filename', type=str, help='DOL file to execute')

    # ELF exec command
    execelf = subparsers.add_parser('execelf', help='Execute ELF file')
    execelf.add_argument('filename', type=str, help='ELF file to execute')

    # RESET command
    reset = subparsers.add_parser('reset', help='Reset GCLink server')

    args = parser.parse_args()
    gclink_host = args.host
    gclink_port = args.port

    if args.command in ['execelf', 'execdol']:
        path = Path(args.filename)
        if not path.is_file():
            raise RuntimeError(f"filename {args.filename} doesn't exist")
        if '.' not in path.suffix or path.suffix[1:] not in args.command:
            raise RuntimeError(f"filename {args.filename} doesn't have the required extension")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        print(f"Connecting to GameCube GCLink server at {gclink_host}:{gclink_port}...")
        s.connect((gclink_host, gclink_port))

        if args.command == "reset":
            s.sendall(b"RESET")
            data = s.recv(1024)

            print(f"Received {data!r}")
            print("Success!")

        elif args.command == "execdol":
            size = path.stat()

            cmd = f"EXECDOL {path.name} {size.st_size}"
            print(f"Command: {cmd}")

            s.sendall(cmd.encode("ascii"))
            data = s.recv(1024)

            ack = data.decode("ascii")
            if ack == "OK":
                with open(str(path), "rb") as f:
                    file_data = f.read()
                    s.sendall(file_data)

                    data = s.recv(1024)
                    ack = data.decode("ascii")
                    if ack != "DOL executed":
                        raise RuntimeError("file not sent correctly!")

                    print("Success!")
            else:
                raise RuntimeError(f"Error! Ack received: {ack}")

        elif args.command == "execelf":
            size = path.stat()

            cmd = f"EXECELF {path.name} {size.st_size}"
            print(f"Command: {cmd}")

            s.sendall(cmd.encode("ascii"))
            data = s.recv(1024)

            ack = data.decode("ascii")
            if ack == "OK":
                with open(str(path), "rb") as f:
                    file_data = f.read()
                    s.sendall(file_data)
                    data = s.recv(1024)

                    ack = data.decode("ascii")
                    if ack != "ELF executed":
                        raise RuntimeError("file not sent correctly!")

                    print("Success!")
            else:
                raise RuntimeError(f"Error! Ack received: {ack}")







