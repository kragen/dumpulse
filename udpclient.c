/* Simple example UDPv4 client, corresponding to the server in
 * udpserver.c.  XXX rewrite this in Python
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

enum {heartbeat_magic = 0xf1, mod_adler = 65521};
/* XXX C&P from dumpulse.c */
static u32 adler32(u8 *p, size_t len)
{
  u32 a = 1, b = 0;
  while (len--) {
    a += *p++;
    b += a;
    if (!(len & 0xff)) {
      /* Note, this is guaranteed to run on the last byte because len == 0 */
      if (a >= mod_adler) a -= mod_adler;
      if (b >= mod_adler) b -= mod_adler;
    }
  }
  return b << 16 | a;
}

struct sockaddr_in sa = {.sin_family = AF_INET};

int health_report(int sock)
{
  u8 reply_buf[512];
  int bytes, i;
  if (-1 == sendto(sock, "AreyouOK", 8, 0, (struct sockaddr*)&sa, sizeof(sa))) {
    perror("sendto");
    return 0;
  }
  bytes = recv(sock, reply_buf, sizeof(reply_buf), 0);
  if (-1 == bytes) {
    perror("recv");
    return 0;
  }

  printf("Health report %d bytes:\n", bytes);
  if (bytes < 4) {
    fprintf(stderr, "too small\n");
    return 0;
  }
  for (i = 4; i < bytes; i += 4) {
    printf("v%d = %d (timestamp %d, from %d)\n",
           i/4 - 1,
           reply_buf[i+3],
           reply_buf[i] | reply_buf[i+1] << 8,
           reply_buf[i+2]);

  }

  printf("Checksum %02x%02x%02x%02x, should be %08x\n",
         reply_buf[3], reply_buf[2], reply_buf[1], reply_buf[0],
         adler32(reply_buf + 4, bytes - 4));
  return 1;
}

int set_var(int sock, int var, int sender, int val)
{
  u8 buf[8] = { 0, 0, 0, 0, heartbeat_magic, var, sender, val };

  u32 checksum = adler32(buf+4, 4);
  buf[0] = checksum;
  buf[1] = checksum >> 8;
  buf[2] = checksum >> 16;
  buf[3] = checksum >> 24;

  if (-1 == sendto(sock, buf, 8, 0, (struct sockaddr*)&sa, sizeof(sa))) {
    perror("sendto");
    return 0;
  }
  return 1;
}

int main(int argc, char **argv)
{
  int fd;

  if (3 != argc && 6 != argc) {
    fprintf(stderr, "Usage: %s 127.0.0.1 9060 or %s 127.0.0.1 9060 3 4 5.\n"
            "The first form requests and displays a health report from\n"
            "the Dumpulse server at 127.0.0.1 UDPv4 port 9060.\n"
            "The second form sends it a packet to set variable 3\n"
            "from source 4 to value 5.\n",
            argv[0], argv[0]);
    return 1;
  }

  sa.sin_port = htons(atoi(argv[2]));
  sa.sin_addr.s_addr = inet_addr(argv[1]);

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == fd) {
    perror("socket");
    return 1;
  }

  if (3 == argc) {
    return !health_report(fd);
  } else {
    return !set_var(fd, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
  }
}
