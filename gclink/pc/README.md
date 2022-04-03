## GCLink PC client

### Prerequisites

 - Python 3
 - GCLink running on the local network (ex: 192.168.1.10), usually port 2323 and not firewalled

### Usage

 - Execute DOL file

    ````bash
    python gclink_client.py --host 192.168.1.10 execdol test.dol
    ````

 - Execute ELF file

    ````bash
    python gclink_client.py --host 192.168.1.10 execelf test.elf
    ````

 - Reset GCLink

    ````bash
    python gclink_client.py --host 192.168.1.10 reset
    ````

### Command line:

````bash
$ python gclink_client.py --help
usage: gclink_client.py [-h] --host HOST [--port PORT]
                        {execdol,execelf,reset} ...

GCLink client by Shazz/TRSi

positional arguments:
  {execdol,execelf,reset}
                        Specific command help
    execdol             Execute DOL file
    execelf             Execute ELF file
    reset               Reset GCLink server

optional arguments:
  -h, --help            show this help message and exit
  --host HOST           GCLink server hostname or ip
  --port PORT           GCLink server port, default 2323
````
