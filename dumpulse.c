/* Implementation of Dumpulse, a dumb heartbeat daemon in 256 bytes of
   RAM and ≈350 bytes of code
 */
#include <stdint.h>
#include <string.h>

#include "dumpulse.h"

enum {heartbeat_magic = 0xf1, mod_adler = 65521};

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

static void store_little_endian_u16(u8 *p, u16 v)
{
  p[0] = 0xff & v;
  p[1] = 0xff & (v >> 8);
}

static void store_little_endian_u32(u8 *p, u32 v)
{
  store_little_endian_u16(p, v);
  store_little_endian_u16(p+2, v >> 16);
}

static u32 fetch_little_endian_u32(u8 *p)
{
  return (u32)p[0]
    | (u32)p[1] << 8
    | (u32)p[2] << 16
    | (u32)p[3] << 24
    ;
}

static u8 update_entry(dumpulse *p, u8 entry, u8 from, u8 value)
{
  u8 *item;
  if (entry > dumpulse_n_variables) return 0;
  item = p->table + dumpulse_checksum_len + dumpulse_entry_size * entry;
  store_little_endian_u16(item, dumpulse_get_timestamp());
  item[2] = from;
  item[3] = value;
  return 1;
}

static u32 adler32(u8 *p, size_t len)
{
  u32 a = 1, b = 0;
  while (len--) {
    a += *p++;
    if (a >= mod_adler) a -= mod_adler;
    b += a;
    if (b >= mod_adler) b -= mod_adler;
  }
  return b << 16 | a;
}

static u8 process_heartbeat(dumpulse *p, u8 *data)
{
  u32 expected = fetch_little_endian_u32(data);
  u8 *payload = data + dumpulse_checksum_len;
  u32 checksum = adler32(payload,
                         dumpulse_timestamp_len
                         + dumpulse_id_len
                         + dumpulse_value_len);
  if (checksum != expected) return 0;
  return update_entry(p, payload[1], payload[2], payload[3]);
}

static void send_response(dumpulse *p, void *context)
{
  store_little_endian_u32(p->table, adler32(
    (u8*)p->table + dumpulse_checksum_len,
    sizeof(p->table) - dumpulse_checksum_len));
  dumpulse_send_packet(context, (char*)p->table, sizeof(p->table));
}

u8 dumpulse_process_packet(dumpulse *p, char *data, void *context)
{
  u8 *d = (u8*)data;
  if (heartbeat_magic == d[4]) {
    return process_heartbeat(p, d);
  } else if (0 == memcmp(d, "AreyouOK", 8)) {
    send_response(p, context);
    return 1;
  }
  return 0;
}
