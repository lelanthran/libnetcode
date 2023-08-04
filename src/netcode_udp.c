
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
      netcode_util_init (); \
      initialised = true;\
   }\
} while (0)

#endif

/* ***************************************************************** */
#ifdef PLATFORM_POSIX
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

#ifdef PLATFORM_Windows
#  define VALID_SOCKET(x) (x!=INVALID_SOCKET)
#else
#  define INVALID_SOCKET  -1
#  define VALID_SOCKET(x) (x >= 0)
#endif

socket_t netcode_udp_socket (uint16_t listen_port, const char *default_host)
{
   socket_t sockfd;
   struct sockaddr_in addr;
   struct hostent *host_addr = NULL;

   memset (&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons (listen_port);

   if (default_host) {
      if ((host_addr = gethostbyname (default_host))!=NULL) {
         memcpy (&addr.sin_addr.s_addr, host_addr->h_addr_list[0],
                 sizeof (addr.sin_addr.s_addr));
      } else {
         return -1;
      }
   } else {
      addr.sin_addr.s_addr = INADDR_ANY;
   }

   sockfd = socket (AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
   if (!(VALID_SOCKET(sockfd))) {
      return -1;
   }

   if ((bind (sockfd, (struct sockaddr*)&addr, sizeof (addr))) < 0) {
      return -1;
   }
   return sockfd;
}


int64_t netcode_udp_wait (socket_t fd, char **remote_host, uint16_t *remote_port,
                          uint8_t **buf, uint32_t *buflen,
                          uint32_t timeout)
{
   bool error = true;
   uint32_t retval = (uint32_t)-1;

   struct timeval tv = { timeout , 0 };
   int error_code = 0;
   socklen_t error_code_len = sizeof error_code;
   struct sockaddr_in addr_remote;
   socklen_t addr_remote_len = sizeof addr_remote;
#ifdef PLATFORM_Windows
   char *tmp = NULL;
#endif

   memset (&addr_remote, 0xff, sizeof (addr_remote));

   SAFETY_CHECK;

   *buf = NULL;
   *buflen = 0;

#ifdef PLATFORM_Windows
   getsockopt (fd, SOL_SOCKET, SO_ERROR,(char *)&error_code, (int *)&error_code_len);
#else
   getsockopt (fd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_len);
#endif
   if (error_code!=0)
      goto errorexit;

   fd_set fds;
   FD_ZERO (&fds);
   FD_SET (fd, &fds);
   int selresult = select ((int)fd + 1, &fds, NULL, NULL, &tv);
   if (selresult > 0) {
      netcode_util_clear_errno ();
#ifdef PLATFORM_Windows
      int max_size = 70 * 1024;
      tmp = malloc (max_size);
      int64_t r = recvfrom (fd, tmp, max_size, MSG_DONTWAIT | MSG_PEEK,
                              (struct sockaddr *)&addr_remote, (int *)&addr_remote_len);
#else
      int64_t r = recvfrom (fd, NULL, 0, MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC,
                              (struct sockaddr *)&addr_remote, &addr_remote_len);
#endif

      // An error occurred, return errorcode
      if (r < 0 ) {
         NETCODE_UTIL_LOG ("First possible error: %i, %" PRIi64 "\n",
                  errno, r);
         goto errorexit;
      }

      // Copy the addr info

      if (remote_host) {
         *remote_host = netcode_util_sockaddr_to_str ((const struct sockaddr *)&addr_remote);
      }
      if (remote_port) {
         *remote_port = ntohs (addr_remote.sin_port);
      }

      // Zero length datagram received. We're returning nothing except the
      // remote peer's address info.
      if (r == 0) {
         retval = *buflen;
         error = false;
         goto errorexit;
      }

      // Valid length in r. Reallocate the dst buffer and try again.
      *buflen = (uint32_t)r;
      if (!(*buf = malloc (*buflen))) {
         goto errorexit;
      }
#ifdef PLATFORM_Windows
      memcpy (*buf, tmp, *buflen);
#else
      r = recvfrom (fd, *buf, *buflen, MSG_DONTWAIT, NULL, NULL);
#endif

      if ((uint32_t)r != *buflen) {
         goto errorexit; // Underlying error in the socket implementation
      }
   }
   if (selresult==0) {
      retval = *buflen;
      error = false;
      goto errorexit;
   }
   if (selresult < 0) {
      goto errorexit;
   }

   retval = *buflen;
   error = false;

errorexit:

#ifdef PLATFORM_Windows
   free (tmp);
#endif

   if (error) {
      free (*buf);
      *buf = NULL;
      *buflen = 0;
      if (remote_host) {
         free (*remote_host);
         *remote_host = NULL;
      }
      retval = (uint32_t)-1;
   }

   return retval;
}

static int64_t netcode_udp_send_single (socket_t fd,
                                        const char *remote_host, uint16_t port,
                                        void *buf, uint32_t buflen)
{
   int64_t txed = 0;
   int flags = 0;
   struct sockaddr_in dest_addr;
   struct hostent *host_addr = NULL;

   memset (&dest_addr, 0, sizeof dest_addr);
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons (port);

   if (remote_host && port) {
      if ((host_addr = gethostbyname (remote_host))!=NULL) {
         memcpy (&dest_addr.sin_addr.s_addr, host_addr->h_addr_list[0],
                 sizeof dest_addr.sin_addr.s_addr);
      } else {
         NETCODE_UTIL_LOG ("gethostbyname(%s) failure\n", remote_host);
         return (uint32_t)-1;
      }

#ifdef PLATFORM_Windows
      if ((txed = sendto (fd, (char *)buf,  (int)buflen, flags,
                          (const struct sockaddr *)&dest_addr, sizeof (dest_addr)))==-1) {
#else
      if ((txed = sendto (fd, buf, buflen, flags,
                          (const struct sockaddr *)&dest_addr, sizeof (dest_addr)))==-1) {
#endif
         NETCODE_UTIL_LOG ("sendto dest failure\n");
         return (uint32_t)-1;
      }
   } else {
#ifdef PLATFORM_Windows
      if ((txed = sendto (fd, (char *)buf,  (int)buflen, flags, NULL, 0))==-1) {
#else
      if ((txed = sendto (fd, buf,  buflen, flags, NULL, 0))==-1) {
#endif
         NETCODE_UTIL_LOG ("sendto() connected failure\n");
         return (uint32_t)-1;
      }
   }

   return (uint32_t)txed;
}

int64_t netcode_udp_senda (socket_t fd, const char *remote_host, uint16_t port,
                           uint32_t nbuffers,
                           void **buffers, uint32_t *buffer_lengths)
{
   uint8_t *txbuf = NULL;
   uint32_t txbuf_len = 0;
   uint32_t txbuf_idx = 0;
   int64_t nbytes = 0;

   for (uint32_t i=0; i<nbuffers; i++) {
      txbuf_len += buffer_lengths[i];
   }
   if (!(txbuf = malloc (txbuf_len * (sizeof *txbuf)))) {
      NETCODE_UTIL_LOG ("Error: Out of memory\n");
      return (uint32_t)-1;
   }

   for (uint32_t i=0; i<nbuffers; i++) {
      memcpy (&txbuf[txbuf_idx], buffers[i], buffer_lengths[i]);
      txbuf_idx += buffer_lengths[i];
   }

   nbytes = netcode_udp_send_single (fd, remote_host, port, txbuf, txbuf_len);
   free (txbuf);
   return nbytes;
}

int64_t netcode_udp_send (socket_t fd, const char *remote_host, uint16_t port,
                          void *buf1, uint32_t buflen1,
                          ...)
{
   va_list ap;
   va_start (ap, buflen1);
   int64_t nbytes = netcode_udp_sendv (fd, remote_host, port, buf1, buflen1, ap);
   va_end (ap);
   return nbytes;
}

int64_t netcode_udp_sendv (socket_t fd, const char *remote_host, uint16_t port,
                           void *buf1, uint32_t buflen1,
                           va_list ap)
{
   int64_t nbytes = 0;
   void **txbuffers = NULL;
   uint32_t *txbuffer_lengths = NULL;
   uint32_t nbuffers = 0;
   va_list vc;
   void *tmp = buf1;
   uint32_t tmplen = 0;

   va_copy (vc, ap);
   for (uint32_t i=0; tmp; i++) {
      nbuffers++;
      tmp = va_arg (vc, void *);
      tmplen = va_arg (vc, uint32_t);
   }
   (void)tmplen;
   va_end (vc);

   if (!(txbuffers = calloc (nbuffers + 1, sizeof *txbuffers))) {
      NETCODE_UTIL_LOG ("Error: Out of memory\n");
      return (uint32_t)-1;
   }

   if (!(txbuffer_lengths = calloc (nbuffers + 1, sizeof *txbuffer_lengths))) {
      free (txbuffers);
      NETCODE_UTIL_LOG ("Error: Out of memory\n");
      return (uint32_t)-1;
   }

   va_copy (vc, ap);
   for (uint32_t i=0; buf1; i++) {
      txbuffers[i] = buf1;
      txbuffer_lengths[i] = buflen1;
      buf1 = va_arg (vc, void *);
      buflen1 = va_arg (vc, uint32_t);
   }
   va_end (vc);

   nbytes = netcode_udp_senda (fd, remote_host, port,
                               nbuffers,
                               txbuffers, txbuffer_lengths);

   free (txbuffers);
   free (txbuffer_lengths);

   return nbytes;
}

