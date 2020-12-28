
#include "netcode_udp.h"

int netcode_udp_socket (uint16_t port, const char *host)
{
   (void)port;
   (void)host;
   return -1;
}


size_t netcode_udp_wait (int fd, char **remote_host,
                         uint8_t **buf, size_t *buflen,
                         size_t timeout)
{
   (void)fd;
   (void)remote_host;
   (void)buf;
   (void)buflen;
   (void)timeout;
   return (size_t)-1;
}

size_t netcode_udp_send (int fd, char *remote_host,
                         uint8_t *buf, size_t buflen)
{
   (void)fd;
   (void)remote_host;
   (void)buf;
   (void)buflen;

   return (size_t)-1;
}

int netcode_udp_errno (void)
{
   return -1;
}

const char *netcode_udp_strerror (int error)
{
   (void)error;
   return "UNIMPLEMENTED";
}

