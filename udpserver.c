/* Example UDPv4 server for dumpulse.
 *
 * You wouldn’t want to run this on the internet or a LAN, where
 * malicious actors might be present, but it serves as an example.
 */
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include "dumpulse.h"

#ifdef DEBUG
#define debug(x) printf x
#else
#define debug(x)
#endif

typedef struct {
  int fd;
  struct sockaddr sa;
  socklen_t len;
} ctx;

uint16_t dumpulse_get_timestamp()
{
  return (uint16_t)time(NULL);
}

void dumpulse_send_packet(void *context, char *data, size_t len)
{
  ctx *c = (ctx*)context;
  debug(("Sending a %d-byte packet.\n", (int)len));
  sendto(c->fd, data, len, 0, &c->sa, c->len);
}

dumpulse dump;

int main(int argc, char **argv)
{
  int yes = 1;
  ctx c;
  uint8_t buf[8];
  struct sockaddr_in me = {
    .sin_family = AF_INET,
    .sin_addr = {0},
    .sin_zero = {0},
  };

  if (argc != 2) {
    fprintf(stderr,
            "Usage: %s 9060, where 9060 is the UDP port to listen on.\n",
            argv[0]);
    return 1;
  }
  me.sin_port = htons(atoi(argv[1]));

  c.fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == c.fd) {
    perror("socket");
    return 1;
  }

  if (-1 == bind(c.fd, (struct sockaddr*)&me, sizeof me)) {
    perror("bind");
    return 1;
  }

  if (-1 == setsockopt(c.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) {
    perror("setsockopt");
    return 1;
  }

  printf("Waiting for UDPv4 packets on port %d.\n", ntohs(me.sin_port));
  for (;;) {
    c.len = sizeof(c.sa);
    if (8 == recvfrom(c.fd, buf, sizeof(buf), 0, &c.sa, &c.len)) {
      debug(("Got a packet %02x %02x %02x %02x %02x %02x %02x %02x.\n",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));
      if (dumpulse_process_packet(&dump, (char*)buf, &c)) {
        debug(("It was handled.\n"));
      } else {
        debug(("It was not handled.\n"));
      }
    }
  }
}
