#!/usr/bin/python3
"Python3 interface to Dumpulse server code, for testing."
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


so = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__) or '.',
                                   'dumpulse.so'))

# Size of the internal data structure.
n = 260


get_timestamp_t = CFUNCTYPE(c_ushort)
send_packet_t = CFUNCTYPE(c_ushort, c_void_p, POINTER(c_char), c_ulong)


class dumpulse_so(Structure):
    _fields_ = [
        ('context', c_void_p),
        ('get_timestamp', get_timestamp_t),
        ('send_packet', send_packet_t),
        ('p', POINTER(c_char * n))
    ]


dumpulse_process_packet_so = so.dumpulse_process_packet_so
dumpulse_process_packet_so.argtypes = [POINTER(dumpulse_so), c_char_p]
dumpulse_process_packet_so.restype = c_ubyte


class Dumpulse:
    def __init__(self):
        self.buf = create_string_buffer(n)

    def process_packet(self, packet, get_timestamp, send_packet):
        """Invoke Dumpulse.

        packet — an 8-byte packet
        get_timestamp — a Python function returning an int
        send_packet — a Python function taking a bytes object
        """
        assert len(self.buf) == n
        assert len(packet) == 8 and isinstance(packet, bytes)

        def send_packet_wrapper(context, pointer, length):
            send_packet(pointer[:length])
            return 1     # not sure how to declare a void function yet

        struct = dumpulse_so(context=None,
                             get_timestamp=get_timestamp_t(get_timestamp),
                             send_packet=send_packet_t(send_packet_wrapper),
                             p=pointer(self.buf))
        return so.dumpulse_process_packet_so(pointer(struct), packet)


if __name__ == '__main__':
    import struct, zlib
    x = Dumpulse()
    # Query before any sets
    print(x.process_packet(b"AreyouOK",
                           lambda: (print("timestamp"), 12345)[1],
                           lambda data: print(data)))
    # Valid set
    payload = struct.pack("BBBB", 0xf1, 3, 4, 5)
    req = struct.pack("<L", zlib.adler32(payload)) + payload
    print(x.process_packet(req,
                           lambda: (print("timestamp"), 12345)[1],
                           lambda data: print(data)))
    # Invalid data
    print(x.process_packet(b"12345678",
                           lambda: (print("timestamp"), 12345)[1],
                           lambda data: print(data)))
    # Query showing the set
    print(x.process_packet(b"AreyouOK",
                           lambda: (print("timestamp"), 12345)[1],
                           lambda data: print(data)))
