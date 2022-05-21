import socket
import ipaddress
import sys
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

def cmd_upload(gclink_host, gclink_port, cmd_type: str, path: Path, dest: str = None):

    cmd_types = {
        "dol": "EXECDOL",
        "elf": "EXECELF",
        "copy_dol": "COPYDOL",
    }
    if cmd_type not in cmd_types:
        raise RuntimeError(f"Unknown type: {cmd_type}")

    # check file exist and is not empty before connecting
    if not path.is_file():
        raise RuntimeError(f"filename {args.filename} doesn't exist")

    if (cmd_type in ['dol', 'elf']) and ('.' not in path.suffix or path.suffix[1:] != cmd_type):
        raise RuntimeError(f"filename {args.filename} doesn't have the required extension: {cmd_type}")

    if path.stat().st_size == 0:
        raise RuntimeError(f"File size cannot be 0 bytes")

    # connect
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect((gclink_host, gclink_port))

            # send command
            if cmd_type == "copy_dol":
                cmd = f"{cmd_types[cmd_type]} {path.name} {dest} {path.stat().st_size}"
            else:
                cmd = f"{cmd_types[cmd_type]} {path.name} {path.stat().st_size}"
            print(f"Command: {cmd}")

            s.sendall(cmd.encode("ascii"))

            # get ack
            s.settimeout(5)
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
                    if "executed" not in ack or "copied" not in ack :
                        raise RuntimeError(f"file not sent correctly! Error: {ack}")

                    print("Success!")
            else:
                raise RuntimeError(f"Error! Received: {ack}")

    except KeyboardInterrupt:
        print("Exiting!")
        sys.exit()
    except socket.timeout:
        print("Timeout, exiting!")
        sys.exit()


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

    # DOL exec command
    execdol = subparsers.add_parser('copydol', help='Copy DOL file')
    execdol.add_argument('dest', type=str, help='storage destination', choices=['sd2sp2', 'gecko_a', 'gecko_b'])
    execdol.add_argument('filename', type=str, help='DOL file to copy')

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
        cmd_upload(gclink_host, gclink_port, "dol", Path(args.filename))

    elif args.command == "execelf":
        cmd_upload(gclink_host, gclink_port, "elf", Path(args.filename))

    elif args.command == "copydol":
        cmd_upload(gclink_host, gclink_port, "copy_dol", Path(args.filename), args.dest)








