
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
   bool error = true;
   size_t retval = (size_t)-1;

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
   if (error_code!=0)
      goto errorexit;

   fd_set fds;
   FD_ZERO (&fds);
   FD_SET (fd, &fds);
   int selresult = select (fd + 1, &fds, NULL, NULL, &tv);
   if (selresult > 0) {
      netcode_util_clear_errno ();
      ssize_t r = recvfrom (fd, NULL, 0, MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC,
                            (struct sockaddr *)&addr_remote, &addr_remote_len);

      // An error occurred, return errorcode
      if (r < 0 ) {
         goto errorexit;
      }

      // Copy the addr info
      *remote_host = malloc (17);
      if (!*remote_host) {
         goto errorexit;
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
      if (r == 0) {
         error = false;
         goto errorexit;
      }

      // Valid length in r. Reallocate the dst buffer and try again.
      *buflen = (size_t)r;
      if (!(*buf = malloc (*buflen))) {
         goto errorexit;
      }
      r = recvfrom (fd, *buf, *buflen, MSG_DONTWAIT, NULL, NULL);

      if ((size_t)r != *buflen) {
         goto errorexit; // Underlying error in the socket implementation
      }
   }
   if (selresult==0) {
      error = false;
      goto errorexit;
   }
   if (selresult < 0) {
      goto errorexit;
   }

   retval = *buflen;
   error = false;

errorexit:

   if (error) {
      free (*buf);
      *buf = NULL;
      *buflen = 0;
      free (*remote_host);
      *remote_host = NULL;
      retval = (size_t)-1;
   }

   return retval;
}

size_t netcode_udp_send (int fd, char *remote_host, uint16_t port,
                         uint8_t *buf, size_t buflen)
{
   ssize_t txed = 0;
   int flags = 0;
   struct sockaddr_in dest_addr;
   struct hostent *host_addr = NULL;

   if (remote_host && port) {
      memset (&dest_addr, 0, sizeof dest_addr);
      dest_addr.sin_family = AF_INET;
      dest_addr.sin_port = htons (port);

      if ((host_addr = gethostbyname (remote_host))!=NULL) {
         memcpy (&dest_addr.sin_addr.s_addr, host_addr->h_addr_list[0],
                 sizeof dest_addr.sin_addr.s_addr);
      } else {
         return (size_t)-1;
      }

      if ((txed = sendto (fd, buf, buflen, flags,
                          (const struct sockaddr *)&dest_addr, sizeof (dest_addr)))==-1) {
         return (size_t)-1;
      }
   } else {
      if ((txed = sendto (fd, buf, buflen, flags, NULL, 0))) {
         return (size_t)-1;
      }
   }

   return (size_t)txed;
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

