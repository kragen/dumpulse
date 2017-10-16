/* Code to run Dumpulse as a shared library (.so or DLL).  This is
   mostly intended for testing.
 */

#include <stdint.h>
#include <stdlib.h>

#include "dumpulse.h"

#ifdef __GNUC__
#define thread_local __thread
#else
#define thread_local      /* Anyone want to make this work on VC++? */
#endif

static thread_local dumpulse_so *current;

uint8_t dumpulse_process_packet_so(dumpulse_so *p, char *data)
{
  current = p;
  uint8_t rv = dumpulse_process_packet(p->p, data, p->context);
  current = 0;
  return rv;
}

uint16_t dumpulse_get_timestamp()
{
  return current->get_timestamp();
}

void dumpulse_send_packet(void *context, char *data, size_t len)
{
  current->send_packet(context, data, len);
}
