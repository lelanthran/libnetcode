
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


int netcode_tcp_server (size_t port)
{
   /* ****************************************
    * 1. Call socket() to create a socket
    * 2. Call bind() to bind to a local address
    * 3. Call listen() to wait for incoming connection
    * 4. Call accept() to get the clients connection.
    */
   SAFETY_CHECK;
   struct sockaddr_in addr;

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons (port);
   int fd = -1;
   if (port==0) {
      return -1;
   }
   fd = socket (AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
   if (fd<0) {
      // NETCODE_UTIL_LOG ("socket() failed\n");
      return -1;
   }

   if (bind (fd, (struct sockaddr *)&addr, sizeof addr)!=0) {
      // NETCODE_UTIL_LOG ("bind() failed\n");
      close (fd); fd = -1;
      return -1;
   }
   if (listen (fd, 1)!=0) {
      // NETCODE_UTIL_LOG ("listen() failed\n");
      close (fd); fd = -1;
      return -1;
   }
   return fd;
}

int netcode_tcp_accept (int fd, size_t timeout, char **addr, uint16_t *port)
{
   struct sockaddr_in ret;
   socklen_t retlen = sizeof ret;
   int retval = -1;

   memset(&ret, 0xff, sizeof ret);

   struct timeval tv = { timeout , 0 };
   fd_set fds[3];
   for (size_t i=0; i<sizeof fds/sizeof fds[0]; i++) {
      FD_ZERO (&fds[i]);
      FD_SET (fd, &fds[i]);
   }
   int r = select (fd + 1, &fds[0], &fds[1], &fds[2], &tv);
   if (r==0) {
      return 0;
   }
   retval = accept4 (fd, (struct sockaddr *)&ret, &retlen, SOCK_CLOEXEC);
   if (retval <= 0) {
      return -1;
   }

   /* This should be performed by the caller on every accepted socket.
#ifdef OSTYPE_Darwin
   int optval = SO_NOSIGPIPE;
   if (setsockopt (retval, SOL_SOCKET, &optval, sizeof optval)!=0) {
      fprintf (stderr, "setsockopt(NOSIGPIPE) failure.\n");
      close (retval);
      return -1;
   }
#endif
   */

   if (addr) {
      *addr = malloc (17);
      if (!*addr) {
         return retval;
      }

      uint8_t bytes[4];
      bytes[3] = (ret.sin_addr.s_addr >> 24) & 0xff;
      bytes[2] = (ret.sin_addr.s_addr >> 16) & 0xff;
      bytes[1] = (ret.sin_addr.s_addr >>  8) & 0xff;
      bytes[0] = (ret.sin_addr.s_addr      ) & 0xff;
      sprintf (*addr, "%u.%u.%u.%u", bytes[0],
                                     bytes[1],
                                     bytes[2],
                                     bytes[3]);

   }

   if (port) {
      *port = ntohs (ret.sin_port);
   }
   return retval;
}

int netcode_tcp_connect (const char *server, size_t port)
{
   /* ****************************************
    * 0. Resolve the server name.
    * 1. Call socket() to create a new socket.
    * 2. Call connect() to connect to a remote server.
    */
   SAFETY_CHECK;
   // Resolving server name
   struct hostent *serv_addr = gethostbyname (server);
   struct sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons (port);
   addr.sin_addr.s_addr = inet_addr (server);

   if (!serv_addr) {
      return -1;
   }

   memcpy (&addr.sin_addr.s_addr, serv_addr->h_addr_list[0],
         sizeof addr.sin_addr.s_addr);

   // Creating socket endpoint
   int fd = socket (AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
   if (fd<0) return -1;

   // Connecting endpoint to the server
   if (connect (fd, (struct sockaddr *)&addr, sizeof addr)!=0) {
      close (fd);
      return -1;
   }

   // Returning the connected fd to be used for reading/writing
   return fd;
}

int netcode_tcp_close (int fd)
{
   /* ****************************************
    * 1. Call close() on the socket descriptor
    */
   SAFETY_CHECK;
   shutdown (fd, SHUT_RDWR);
   return close (fd);
}

size_t netcode_tcp_write (int fd, const void *buf, size_t len)
{
   SAFETY_CHECK;
   // NETCODE_UTIL_LOG ("sending %zu bytes\n", len);
   ssize_t retval = SEND (fd, buf, len);
   if (retval<0) return (size_t)-1;
   return retval;
}

size_t netcode_tcp_read (int fd, void *buf, size_t len, size_t timeout)
{
   size_t idx = 0;
   struct timeval tv = { timeout , 0 };
   unsigned char *buffer = buf;
   int countdown = 2;
   int error_code = 0;
   socklen_t error_code_len = sizeof error_code;

   getsockopt (fd,
               SOL_SOCKET, SO_ERROR, &error_code, &error_code_len);
   if (error_code!=0) return (size_t)-1;
   SAFETY_CHECK;
   // NETCODE_UTIL_LOG ("Attempting to read %zu bytes\n", len);
   do {
      fd_set fds;
      FD_ZERO (&fds);
      FD_SET (fd, &fds);
      int selresult = select (fd + 1, &fds, NULL, NULL, &tv);
      if (selresult>0) {
         netcode_util_clear_errno ();
         ssize_t r = recv (fd, &buffer[idx], len-idx, MSG_DONTWAIT);

         // Return error immediately if an error is detected. Reading zero
         // bytes from a socket that caused a select() to return means
         // that the other side has disconnected.
         if (netcode_util_errno ()) return idx ? idx : (size_t)-1;
         if (r == -1) return idx ? idx : (size_t)-1;
         if (r ==  0) return idx ? idx : (size_t)-1;

         idx += (size_t)r;
         // NETCODE_UTIL_LOG ("read %zu bytes\n", idx);
      }
      if (selresult==0) {
         countdown--;
         continue;
      }
      if (selresult<0) {
         return (size_t)-1;
      }
   } while (idx<len && countdown);
   return idx;
}

