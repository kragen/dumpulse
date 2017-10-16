#!/usr/bin/python3
# -*- coding: utf-8 -*- (for Python2)
"UDP client for Dumpulse, primarily for debuggging."

import argparse
import socket
import struct
from zlib import adler32


# The query packet to send to Dumpulse server to get a health report.
query_packet = b"AreyouOK"


def _variable_settings(p):
    "Yields tuples like those of variable_settings, without checking checksums."

    for v in range(len(p)//4-1):
        timestamp, sender, value = struct.unpack("<HBB", p[4*(v+1):][:4])
        yield v, timestamp, sender, value


def parse_health_report(health_report_bytes):
    """Return a variable settings list and the expected and received checksum.

    The variable settings list is the same list returned by the
    variable_settings function, which you should probably call instead
    for most non-debugging purposes.

    """
    p = health_report_bytes
    checksum, = struct.unpack("<L", p[:4])
    expected = adler32(p[4:])
    return list(_variable_settings(p)), expected, checksum


def variable_settings(health_report_bytes):
    """Returns a list of (variable number, timestamp, sender, value) tuples.

    Raises ValueError if this is not a valid health report packet.

    """
    settings, expected, received = parse_health_report(health_report_bytes)
    if expected != received:
        raise ValueError(health_report_bytes, expected, received)
    return settings


def get_health_report(socket_object):
    socket_object.send(query_packet)
    p = socket_object.recv(2048)
    print("Health report of {} bytes:".format(len(p)))

    settings, expected, checksum = parse_health_report(p)
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

    for v, timestamp, sender, value in settings:
        print("v{} = {} at {} from {}".format(v, value, timestamp, sender))


def set_packet(variable, sender, value):
    "Construct a set-variable request packet and return it as bytes."
    payload = struct.pack("BBBB", 0xf1, variable, sender, value)
    return struct.pack("<L", adler32(payload)) + payload


def set_variable(socket_object, variable, sender, value):
    "Send a set-variable request to a Dumpulse server."
    socket_object.send(set_packet(variable, sender, value))


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
