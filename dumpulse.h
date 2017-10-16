/* #include stdint.h and something that defines size_t before this */

enum {
  dumpulse_n_variables = 64,
  /* bytes: */
  dumpulse_checksum_len = 4,
  dumpulse_timestamp_len = 2,
  dumpulse_id_len = 1,
  dumpulse_value_len = 1,
  dumpulse_entry_size = ( dumpulse_timestamp_len
                          + dumpulse_id_len
                          + dumpulse_value_len )
};

/* The dumpulse data structure; fill with zeros before use */

typedef struct {
  unsigned char table[dumpulse_checksum_len
                      + dumpulse_n_variables * dumpulse_entry_size];
} dumpulse;

/* The dumpulse entry point; see README.md for details */
/* XXX no length */

uint8_t dumpulse_process_packet(dumpulse *p, char *data, void *context);

/* For embedded use, you must provide these two functions */

uint16_t dumpulse_get_timestamp(void);
void dumpulse_send_packet(void *context, char *data, size_t len);

/* If you instead want to dynamically link dumpulse and pass it
   function pointers, you can use the shared library as follows. */

typedef uint16_t (*dumpulse_get_timestamp_t)(void);
typedef void (*dumpulse_send_packet_t)(void*, char*, size_t);
typedef struct {
  void *context;
  dumpulse_get_timestamp_t get_timestamp;
  dumpulse_send_packet_t send_packet;
  dumpulse *p;
} dumpulse_so;

uint8_t dumpulse_process_packet_so(dumpulse_so *p, char *data);
