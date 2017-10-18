/* Benchmark running Dumpulse in a loop to see how fast it can handle
 * heartbeat packets.  On my laptop, it takes about 28 nanoseconds per
 * packet compiled with -Os, or 15 nanoseconds if compiled with -O5.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "dumpulse.h"

uint16_t dumpulse_get_timestamp()
{
  return 12345;
}

void dumpulse_send_packet(void *context, char *data, size_t len)
{
  while (len--) printf("%02x%s", *data++, len&3 ? " " : len&15 ? "  " : "\n");
}

dumpulse d;

int main(int argc, char **argv)
{
  int i;
  /* This is a valid heartbeat packet.  Absent CPU caching effects,
   * all valid heartbeat packets should be processed in the same
   * amount of time, because Dumpulse itself does not do any caching,
   * and GCC is not doing whole-program or link-time optimization.
   */
  char *req = "\x05k\x01\xce\xf1>KS";
  for (i = 0; i < 1000*1000*1000; i++) {
    if (!dumpulse_process_packet(&d, req, 0)) abort();      
  }
  /* Process a health-report packet; the result shows that (at least
   * one of) the above heartbeat packets were processed. */
  dumpulse_process_packet(&d, "AreyouOK", 0);
  return 0;
}
