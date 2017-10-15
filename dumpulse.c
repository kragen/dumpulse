/* Implementation of Dumpulse, a dumb heartbeat daemon in 256 bytes of
   RAM and ≈245 bytes of code
 */
#include <stdint.h>
#include <string.h>

#include "dumpulse.h"

#ifdef __GNUC__
#define inline __inline__
#else
#define inline
#endif

enum {
  heartbeat_magic = 0xf1,
  mod_adler = 65521
};

static inline void store_little_endian_u16(char *p, uint16_t v)
{
  p[0] = 0xff & v;
  p[1] = 0xff & (v >> 8);
}

static inline void store_little_endian_u32(char *p, uint32_t v)
{
  store_little_endian_u16(p, v);
  store_little_endian_u16(p+2, v >> 16);
}

static inline uint32_t fetch_little_endian_u32(char *p)
{
  uint8_t *q = p;
  return (uint32_t)q[0]
    | (uint32_t)q[1] << 8
    | (uint32_t)q[2] << 16
    | (uint32_t)q[3] << 24
    ;
}

static inline uint8_t update_entry(dumpulse *p,
                                   uint16_t entry,
                                   uint8_t from,
                                   uint8_t value)
{
  char *item;
  if (entry > dumpulse_n_variables) return 0;
  item = p->table + dumpulse_checksum_len + dumpulse_entry_size * entry;
  store_little_endian_u16(item, dumpulse_get_timestamp());
  item[2] = from;
  item[3] = value;
  return 1;
}

static inline uint32_t adler32(char *p, size_t len)
{
  uint8_t *data = p;
  uint32_t a = 1, b = 0;
  while (len--) {
    a += *data++;
    b += a;
    if (!(len & 0xfff)) {
      /* Note, this is guaranteed to run on the last byte because len == 0 */
      if (a >= mod_adler) a -= mod_adler;
      if (b >= mod_adler) b -= mod_adler;
    }
  }
  return b << 16 | a;
}

static inline uint8_t process_heartbeat(dumpulse *p, char *data)
{
  uint32_t expected = fetch_little_endian_u32(data);
  uint8_t *payload = data + dumpulse_checksum_len;
  uint32_t checksum = adler32(payload,
                              dumpulse_timestamp_len
                              + dumpulse_id_len
                              + dumpulse_value_len);
  if (checksum != expected) return 0;
  return update_entry(p, payload[1], payload[2], payload[3]);
}

static inline void send_response(dumpulse *p, void *context)
{
  store_little_endian_u32(p->table, adler32(
    p->table + dumpulse_checksum_len,
    sizeof(p->table) - dumpulse_checksum_len));
  dumpulse_send_packet(context, p->table, sizeof(p->table));
}

uint8_t dumpulse_process_packet(dumpulse *p, char *data, void *context)
{
  if (heartbeat_magic == data[4]) {
    return process_heartbeat(p, data);
  } else if (0 == memcmp(data, "AreyouOK", 8)) {
    send_response(p, context);
    return 1;
  }
  return 0;
}
