#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "netcode_util.h"

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

const char *hstrerror (int error);

static bool initialised = false;

#define SAFETY_CHECK       do {\
   if (!initialised) {\
      wsa_netcode_init (); \
      initialised = true;\
   }\
} while (0)

static WSADATA xp_wsaData;
static int xp_wsaInitialised = 0;

static bool wsa_netcode_init (void)
{
   if (xp_wsaInitialised > 0)
      return true;

   atexit ((void (*)(void))WSACleanup);

   int result = WSAStartup (MAKEWORD(2,2), &xp_wsaData);
   if (result!=0) {
      NETCODE_UTIL_LOG ("Critical: winsock could not be initiliased - %i\n", result);
      return false;
   }

   xp_wsaInitialised = 1;
   return true;
}

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
const char *hstrerror (int err);

#define SEND(x,y,z)        send (x,y,z, MSG_NOSIGNAL)

#endif


static bool initialised = true;
#define SAFETY_CHECK    ;

#endif

#ifndef SAFETY_CHECK
#error SAFETY_CHECK not defined - platform variable undefined?
#endif


bool netcode_util_init (void)
{
   SAFETY_CHECK;
   return initialised;
}


int netcode_util_clear_errno (void)
{
   errno = 0;
#ifndef PLATFORM_Windows
   h_errno = 0;
#endif
   return 0;
}

int netcode_util_errno (void)
{
#ifdef PLATFORM_Windows
   return WSAGetLastError ();
#else
   return errno ? errno : h_errno;
#endif
}

const char *netcode_util_strerror (int err)
{
#ifdef PLATFORM_Windows
   return strerror (err);
#else
   return errno ? strerror (err) : hstrerror (err);
#endif
}

int netcode_util_close (int fd)
{
   /* ****************************************
    * 1. Call close() on the socket descriptor
    */
   SAFETY_CHECK;
#ifdef PLATFORM_Windows
   shutdown (fd, SD_BOTH);
#else
   shutdown (fd, SHUT_RDWR);
#endif
   return close (fd);
}

