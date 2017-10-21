#!/usr/bin/python3
# -*- coding: utf-8 -*-
"Python interface to Dumpulse server code, for testing."
from __future__ import print_function
from ctypes import (
    CFUNCTYPE,
    POINTER,
    Structure,
    c_char,
    c_char_p,
    c_ubyte,
    c_ulong,
    c_ushort,
    c_void_p,
    cdll,
    create_string_buffer,
    pointer,
)
import os

import client


so = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__) or '.',
                                   'dumpulse.so'))

# Size of the internal data structure.
n = 260


get_timestamp_t = CFUNCTYPE(c_ushort)
send_packet_t = CFUNCTYPE(c_ushort, c_void_p, POINTER(c_char), c_ulong)


class _dumpulse_so(Structure):
    _fields_ = [
        ('context', c_void_p),
        ('get_timestamp', get_timestamp_t),
        ('send_packet', send_packet_t),
        ('p', POINTER(c_char * n))
    ]


_dumpulse_process_packet_so = so.dumpulse_process_packet_so
_dumpulse_process_packet_so.argtypes = [POINTER(_dumpulse_so), c_char_p]
_dumpulse_process_packet_so.restype = c_ubyte


class Dumpulse:
    def __init__(self, get_timestamp, send_packet):
        """
        get_timestamp — a Python function returning an int
        send_packet — a Python function taking a bytes object

        A normal person would probably have made those functions
        methods you could override in a subclass, but whenever I use
        inheritance, I always come to regret it later.

        """
        self.buf = create_string_buffer(n)

        def send_packet_wrapper(context, pointer, length):
            send_packet(pointer[:length])
            return 1     # not sure how to declare a void function yet

        struct = _dumpulse_so(context=None,
                              get_timestamp=get_timestamp_t(get_timestamp),
                              send_packet=send_packet_t(send_packet_wrapper),
                              p=pointer(self.buf))
        self.p = pointer(struct)

    def process_packet(self, packet):
        """Invoke Dumpulse.

        packet — an 8-byte packet
        """
        assert len(self.buf) == n
        assert len(packet) == 8 and isinstance(packet, bytes)

        return _dumpulse_process_packet_so(self.p, packet)


if __name__ == '__main__':
    x = Dumpulse(get_timestamp=lambda: (print("timestamp"), 12345)[1],
                 send_packet=lambda data: print(repr(data)))

    # Query before any sets
    print(x.process_packet(client.query_packet))
    # Valid set
    print(x.process_packet(client.set_packet(3, 4, 5)))
    # Invalid data
    print(x.process_packet(b"12345678"))
    # Query showing the set
    print(x.process_packet(client.query_packet))
