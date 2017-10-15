#!/usr/bin/python3
"UDP client for Dumpulse."

import argparse
import socket
import struct
from zlib import adler32


def get_health_report(socket_object):
    socket_object.send(b"AreyouOK")
    p = socket_object.recv(2048)
    print("Health report of {} bytes:".format(len(p)))

    checksum, = struct.unpack("<L", p[:4])
    expected = adler32(p[4:])
    if checksum == expected:
        print("checksum {:08x} checks OK".format(checksum))
    else:
        # XXX note that this seems to be showing that the checksum I
        # implemented doesn’t match Adler32 from zlib, in particular
        # in the b parameter, for large packets.  It does match for
        # all-zero packets or very small ones.  This suggests that the
        # modulo is rong.  Also it matches when I weaken the
        # conditional.
        print("checksum {:08x} doesn’t match {:08x} in packet".format(
            expected, checksum))

    for v in range(len(p)//4-1):
        timestamp, sender, value = struct.unpack("<HBB", p[4*(v+1):][:4])
        print("v{} = {} at {} from {}".format(v, value, timestamp, sender))

def set_variable(socket_object, variable, sender, value):
    payload = struct.pack("BBBB", 0xf1, variable, sender, value)
    socket_object.send(struct.pack("<L", adler32(payload)) + payload)

def main():
    parser = argparse.ArgumentParser(
        description="With no value to set, displays a health report.")
    parser.add_argument('host')
    parser.add_argument('port', type=int)
    parser.add_argument('-n', '--variable', type=int, default=0,
                        help="ID of variable to set (0–63)")
    parser.add_argument('-s', '--sender', type=int, default=76,
                        help="ID of sender")
    parser.add_argument('-v', '--value', type=int, help="(0–255)")
    args = parser.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect((args.host, args.port))
    
    if args.value is None:
        get_health_report(s)
    else:
        set_variable(s, args.variable, args.sender, args.value)

if __name__ == '__main__':
    main()
