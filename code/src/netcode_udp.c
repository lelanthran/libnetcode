
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "netcode_util.h"
#include "netcode_tcp.h"

/* ***************************************************************** */
#if defined (OSTYPE_Darwin)
#define SOCK_CLOEXEC       (0)
#define accept4(x,y,z,A)   accept (x,y,(int *)z)
#define SEND(x,y,z)        send (x,y,z, 0)
#endif

/* ***************************************************************** */
#ifdef PLATFORM_Windows
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>

#define SOCK_CLOEXEC       (0)
#define MSG_DONTWAIT       (0)
#define close(x)           closesocket (x)
#define accept4(x,y,z,A)   accept (x,y,(int *)z)
#define SEND(x,y,z)        send (x,y,z, 0)
#define read(x,y,z)        recv (x,(char *)y,z, 0)

typedef size_t socklen_t;

static bool initialised = false;

#define SAFETY_CHECK       do {\
   if (!initialised) {\
      netcode_init (); \
      initialised = true;\
   }\
} while (0)

#endif

/* ***************************************************************** */
#ifdef PLATFORM_POSIX
#include <unistd.h>

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#ifndef OSTYPE_Darwin
/* GCC in strict mode leaves out some libraries it really shouldn't.
 * Rather than spend the next twenty minutes figuring out which set
 * of flags or pragmas will make GCC use the arpa/inet header, I'll
 * just put this here.
 */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);

#define SEND(x,y,z)        send (x,y,z, MSG_NOSIGNAL)

#endif


#define SAFETY_CHECK    ;

#endif


#ifndef SAFETY_CHECK
#error SAFETY_CHECK not defined - platform variable undefined?
#endif

#include "netcode_udp.h"

int netcode_udp_socket (uint16_t port, const char *host)
{
   int sockfd;
   struct sockaddr_in addr;
   struct hostent *host_addr = NULL;

   memset (&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons (port);

   if (host) {
      if ((host_addr = gethostbyname (host))!=NULL) {
         memcpy (&addr.sin_addr.s_addr, host_addr->h_addr_list[0],
                 sizeof (addr.sin_addr.s_addr));
      } else {
         return -1;
      }
   } else {
      addr.sin_addr.s_addr = INADDR_ANY;
   }

   if ((sockfd = socket (AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
      return -1;
   }

   if ((bind (sockfd, (struct sockaddr*)&addr, sizeof (addr))) < 0) {
      return -1;
   }
   return sockfd;
}


size_t netcode_udp_wait (int fd, char **remote_host,
                         uint8_t **buf, size_t *buflen,
                         size_t timeout)
{
   struct timeval tv = { timeout , 0 };
   int error_code = 0;
   socklen_t error_code_len = sizeof error_code;
   struct sockaddr_in addr_remote;
   socklen_t addr_remote_len = sizeof addr_remote;

   memset (&addr_remote, 0xff, sizeof (addr_remote));

   SAFETY_CHECK;

   *remote_host = NULL;
   *buf = NULL;
   *buflen = 0;

   getsockopt (fd,
               SOL_SOCKET, SO_ERROR, &error_code, &error_code_len);
   if (error_code!=0) return (size_t)-1;

   fd_set fds;
   FD_ZERO (&fds);
   FD_SET (fd, &fds);
   int selresult = select (fd + 1, &fds, NULL, NULL, &tv);
   if (selresult > 0) {
      netcode_util_clear_errno ();
      ssize_t r = recvfrom (fd, NULL, 0, MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC,
                            (struct sockaddr *)&addr_remote, &addr_remote_len);
      int rc = netcode_util_errno ();

      // An error occurred, return errorcode
      if (rc < 0 ) {
         free (*remote_host);
         *remote_host = NULL;
         free (*buf);
         *buf = NULL;
         *buflen = 0;
         return -1;
      }

      // Copy the addr info
      *remote_host = malloc (17);
      if (!*remote_host) {
         free (*buf);
         *buf = NULL;
         *buflen = 0;
         return -1;
      }

      uint8_t bytes[4];
      bytes[3] = (addr_remote.sin_addr.s_addr >> 24) & 0xff;
      bytes[2] = (addr_remote.sin_addr.s_addr >> 16) & 0xff;
      bytes[1] = (addr_remote.sin_addr.s_addr >>  8) & 0xff;
      bytes[0] = (addr_remote.sin_addr.s_addr      ) & 0xff;

      snprintf (*remote_host, 16, "%u.%u.%u.%u", bytes[0],
                                                 bytes[1],
                                                 bytes[2],
                                                 bytes[3]);

      // Zero length datagram received. We're returning nothing except the
      // remote peer's address info.
      if (rc == 0) {
         return 0;
      }

      // Valid length in r. Reallocate the dst buffer and try again.
      *buflen = (size_t)r;
      if (!(*buf = malloc (*buflen))) {
         return -1;  // Out of memory
      }
      r = recvfrom (fd, *buf, *buflen, MSG_DONTWAIT, NULL, NULL);

      rc = netcode_util_errno ();

      if (r != *buflen || rc != 0) {
         free (*remote_host);
         *remote_host = NULL;
         free (*buf);
         *buf = NULL;
         *buflen = 0;
         return -1;  // Underlying error in the socket implementation
      }
   }
   if (selresult==0) {
      free (*remote_host);
      *remote_host = NULL;
      free (*buf);
      *buf = NULL;
      *buflen = 0;
      return 0;
   }
   if (selresult < 0) {
      free (*remote_host);
      *remote_host = NULL;
      free (*buf);
      *buf = NULL;
      *buflen = 0;
      return (size_t)-1;
   }
   return *buflen;;
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

