Statistical aspects of Dumpulse: Performance and reliability
============================================================

These are almost just raw notes.

Performance
-----------

Roughly, Dumpulse takes about 200 ns to handle a heartbeat message and
8 μs to send a health report, varying of course based on the processor
you run it on.

Compiled for Linux with amd64 with GCC 5.4.0 with `-Os`, dumpulse is
356 bytes of amd64 code and 76 instructions.  (`-fPIC` adds two more bytes.)
It should be similar in weight, though with more instructions, for
most other processors; for example, compiled for the 386 with `-m32`,
it’s 367 bytes and 92 instructions; compiled for the AVR ATTiny88 with
GCC 4.9.2 with `-Os -mmcu=attiny88`, it’s 323 bytes and 157
instructions; compiled for the ATMega328, it’s 329 bytes and 155
instructions.  However, all of these versions will pull in `memcmp`
from libc, and the AVR versions also `__do_copy_data`, if you aren’t
using them already in your program.

These numbers of course do not count the size of the
`dumpulse_get_timestamp()` and `dumpulse_send_packet()` functions you
must provide.

Running the provided `udpserver` on Linux compiled as above under
valgrind and hitting it with different numbers of requests from the
provided `udpclient`, we get the following results:

    | 0 request packets       | 159931 instructions executed |
    | 1 setvar                |                       161006 |
    | 2 setvars               |                       161134 |
    | 3                       |                       161262 |
    | 10                      |                       162158 |
    | 100                     |                       173678 |
    | 1000                    |                       288878 |
    | 1 health report         |                       165158 |
    | 1 report & 1 setvar     |                       166233 |
    | 1 report & 1000 setvars |                       294105 |

We can analyze these as follows in R:

    > c(165158, 166233, 294105) - c(159931, 161006, 288878)
    [1] 5227 5227 5227
    > c(159931, 161006, 161134, 161262, 162158, 173678, 288878) - 161006
    [1]  -1075      0    128    256   1152  12672 127872
    > (c(159931, 161006, 161134, 161262, 162158, 173678, 288878) - 161006) / c(-1, 0, 1, 2, 9, 99, 999)
    [1] 1075  NaN  128  128  128  128  128

From this we can conclude that the health report costs 5227 amd64
instructions, each heartbeat costs 128 instructions, and there’s an
extra one-time cost of 947 instructions for the first heartbeat,
probably related to glibc’s implementation of time().  These numbers
of course don’t include the time spent in the Linux kernel handling
system calls, but they do include time in udpserver’s
`dumpulse_send_packet` and `dumpulse_get_timestamp` functions.

For a more empirical measure, sending a million variable-setting
packets to udpserver resulted in it consuming 0.4 seconds of user CPU
time and 5.9 seconds of system CPU time, according to Linux, running
on a 1.6 GHz Intel Pentium N3700.  Handling nonsense packets took
roughly the same amount of time.  This works out to about 400 ns per
packet, or 6.3 μs if we include the system time.  Roughly, the health
report takes about 40 times as long as processing a heartbeat.

(The above results are with a simple-minded Adler-32 calculation.)

The more bare-bones test in loopbench.c handles the same heartbeat
packet 1 billion times sequentially in 27–29 seconds on my laptop, so
each packet requires some 27–29 nanoseconds; on a single core, my
laptop could thus handle some 35 million heartbeat requests per
second.

Reliability
-----------

Naïvely calculating, the Adler-32 and magic number in the heartbeat
message provide a probability of a random or corrupted 8-byte packet
being incorrectly accepted of 2⁻³², since for either of the two 32-bit
halves of the message you can calculate an other half (I think unique)
that would make it a valid heartbeat message.  In practice the value
may be somewhat higher or lower; the Adler-32 output is biased toward
low-valued bytes such as 1, 2, 3, 4, 5, 6, 7, and 8, though the bias
toward 0 bytes is very slight, and low-valued bytes are also likely to
occur disproportionately in random packets found in the wild.  In
particular, the byte 2 (^B) occurs about ⅔ of the time, and the byte 6
(^F) occurs about ⅓ of the time.

This suggests that if we are running our heartbeat data raw over a CAN
bus carrying constant traffic of 4000 frames per second, we should
expect to see a non-heartbeat frame randomly misrecognized as a
heartbeat frame about once every million seconds, about once every 12
days.  However, in ¾ of those cases, the variable won’t be in the
valid range of 0–63; in the remaining cases, if it’s being written to
a variable that’s being used by an active heartbeat, it will almost
certainly be overwritten by a valid heartbeat before the remote
monitoring process requests a health report.

(The generative test with Hypothesis does generate random binary data
packets and verify that they don’t change values or emit reply
packets.  This test could happen upon a valid packet by chance, but so
far it hasn’t.)

While the health report request message could occur randomly, the
consequence of sending an unnecessary health report is very mild.
Because its fifth byte is “o” and not 0xf1, it will never be confused
with a heartbeat message.


<link rel="stylesheet" href="http://canonical.org/~kragen/style.css" />

<script src="http://canonical.org/~kragen/sw/addtoc.js">
</script>
