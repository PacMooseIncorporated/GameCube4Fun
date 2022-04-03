import socket
import ipaddress
import argparse
from pathlib import Path

VERSION = "0.1"

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

    return value

def cmd_reset(gclink_host, gclink_port):

    # connect
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((gclink_host, gclink_port))

        # send command
        s.sendall(b"RESET")
        data = s.recv(1024)

        # get result status
        print(f"Received {data!r}")
        print("Success!")

def cmd_exec(gclink_host, gclink_port, cmd_type: str, path: Path):

    cmd_types = {
        "dol": "EXECDOL",
        "elf": "EXECELF"
    }
    if cmd_type not in cmd_types:
        raise RuntimeError(f"Unknown type: {cmd_type}")

    # check file exist and is not empty before connecting
    if not path.is_file():
        raise RuntimeError(f"filename {args.filename} doesn't exist")
    if '.' not in path.suffix or path.suffix[1:] not in args.command:
        raise RuntimeError(f"filename {args.filename} doesn't have the required extension")
    if path.stat().st_size == 0:
        raise RuntimeError(f"File size cannot be 0 bytes")

    # connect
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((gclink_host, gclink_port))

        # send command
        cmd = f"{cmd_types[cmd_type]} {path.name} {path.stat().st_size}"
        print(f"Command: {cmd}")
        s.sendall(cmd.encode("ascii"))

        # get ack
        data = s.recv(1024)
        ack = data.decode("ascii")

        # check ack
        if ack == "OK":

            # send file bytes content
            with open(str(path), "rb") as f:
                file_data = f.read()
                s.sendall(file_data)

                # get result status
                data = s.recv(1024)
                ack = data.decode("ascii")
                if "executed" not in ack:
                    raise RuntimeError("file not sent correctly!")

                print("Success!")
        else:
            raise RuntimeError(f"Error! Received: {ack}")


if __name__ == '__main__':

    # Instantiate the parser
    parser = argparse.ArgumentParser(description=f'GCLink client v{VERSION} by Shazz/TRSi')

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

    print(f"GCLink client v{VERSION} by Shazz/TRSi")
    print(f"Connecting to GameCube GCLink server at {gclink_host}:{gclink_port}...")

    if args.command == "reset":
        cmd_reset(gclink_host, gclink_port)

    elif args.command == "execdol":
        cmd_exec(gclink_host, gclink_port, "dol", Path(args.filename))

    elif args.command == "execelf":
        cmd_exec(gclink_host, gclink_port, "elf", Path(args.filename))








