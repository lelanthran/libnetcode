
#include <stdio.h>
#include <errno.h>
#include <string.h>
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
#if defined (PLATFORM_MSYS) || (PLATFORM_WINDOWS) || (P_windows_51)
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

static WSADATA xp_wsaData;
static int xp_wsaInitialised = 0;
static void xp_winInit (void) {
   if (xp_wsaInitialised > 0) return;
   atexit ((void (*)(void))WSACleanup);
   int result = WSAStartup (MAKEWORD(2,2), &xp_wsaData);
   if (result!=0) {
      NETCODE_UTIL_LOG ("Critical: winsock could not be initiliased - %i\n", result);
   } else {
      NETCODE_UTIL_LOG ("winsock initialised\n");
   }

   xp_wsaInitialised = 1;
}

#define SAFETY_CHECK       do {\
   if (xp_wsaInitialised==0) xp_winInit (); \
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


/**
 * \brief Listen for incoming connection
 *
 * Listen for incoming connections on port \a port. Once a listener is
 * established the function will return a file descriptor suitable for use
 * with \a xsock_accept().
 *
 * \sa xsock_connect(), xsock_accept(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_errno(), xsock_clear_errno(), xsock_strerror()
 *
 * @param[in] port The port to listen on
 *
 * @return On success, the socket descriptor of the incoming connection is
 *    returned and can be used with \a xsock_accept(). On error -1 is
 *    returned and \a xsock_errno() will return the specific error number.
 *
 * \a Example:
 * \verbatim
 int main (void)
 {
    #define TEST_PORT      (8080)
    xsock_clear_errno ();
    // Wait for incoming connection
    int fd = xsock_server (TEST_PORT);
    if (fd<=0) {
       fprintf (stderr, "Unable to establish server\n");
       return EXIT_FAILURE;
    }
    int confd = xsock_accept (fd, 10, NULL, NULL);
    if (confd<=0) {
       fprintf (stderr, "No connection received on %u\n", TEST_PORT);
       fprintf (stderr, "err %i, %s\n", xsock_errno (),
                xsock_strerror(xsock_errno()));
       return EXIT_FAILURE;
    }
    // Read a string from the incoming connection
    char buffer[255]; buffer[255] = 0;
    size_t bytes_read = xsock_read (confd, buffer, sizeof buffer, 5);
    if (bytes_read==0) {
       printf ("Timeout\n");
    } else if (bytes_read<0) {
       printf ("Error reading: %s\n", xsock_strerror(xsock_errno()));
    } else {
      printf ("Read '%s'\n", buffer);
    }
    xsock_close (fd);
    xsock_close (confd);
 }
  \endverbatim
 */
int xsock_server (size_t port)
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
      close (fd);
   }
   fd = socket (AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
   if (fd<0) {
      NETCODE_UTIL_LOG ("socket() failed\n");
      return -1;
   }

   if (bind (fd, (struct sockaddr *)&addr, sizeof addr)!=0) {
      NETCODE_UTIL_LOG ("bind() failed\n");
      close (fd); fd = -1;
      return -1;
   }
   if (listen (fd, 1)!=0) {
      NETCODE_UTIL_LOG ("listen() failed\n");
      close (fd); fd = -1;
      return -1;
   }
   return fd;
}

/**
 * \brief Accept incoming connection
 *
 * Wait no more than \a timeout seconds for an incoming connection on port
 * \a port. If a connection is made then the socket descriptor of the
 * incoming connection is returned, otherwise -1 is returned and the
 * error value is set appropriately (see \a xsock_geterrno()).
 *
 *
 * The \a addr and \a port arguments are optional and can be NULL.
 * If not-NULL these arguments specify addresses into which the
 * remote computer's address and port numbers are written. The
 * \a address argument is allocated and must be freed by the
 * caller.
 *
 * If \a timeout expires before a connection is made then the return value
 * is zero and \a addr and \a port are unchanged.
 *
 * NOTE: Writing to a socket that has been closed raises SIGPIPE on BSD
 * platforms, including OS X/Darwin. On these platforms the socket
 * returned by this call must be set using \a setsockopt to ignore
 * SIGPIPE as follows:
 * \verbatim
 *
   int optval = SO_NOSIGPIPE;
   if (setsockopt (retval, SOL_SOCKET, &optval, sizeof optval)!=0) {
      fprintf (stderr, "setsockopt(NOSIGPIPE) failure.\n");
   }
  \endverbatim

 * On Linux the \a xsock_write() call specifically suppresses SIGPIPE. On
 * Windows, writing to a closed socket doesn't generate the SIGPIPE call
 * anyway.
 *
 * \sa xsock_connect(), xsock_server(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_errno(), xsock_clear_errno(), xsock_strerror()
 *
 * @param[in] port The port to listen on
 * @param[in] timeout The timeout in seconds to wait for a connection
 * @param[out] addr If not-NULL, the IP address of the remote party
 *                will be stored at this address. Note that the
 *                caller must free the pointer when it is no longer
 *                used.
 * @param[out] port If not-NULL, the port number of the remote party
 *                will be stored at this address. This pointer must
 *                point to a variable and will not be allocated by
 *                \a xsock_accept().
 *
 * @return On success, the socket descriptor of the incoming connection is
 *    returned and can be used with \a xsock_read(), \a xsock_write() and
 *    \a xsock_close(). The \a addr argument, if not-NULL, will be
 *    allocated and populated with a human-readable format of the address
 *    of the remote computer. The caller must free this argument.
 *    The \a port argument, if not NULL,
 *    will be populated with the port number of the remote computer.
 *
 *    On timeout, zero is returned. On error -1 is
 *    returned and \a xsock_errno() will return the specific error number.
 *
 *
 * \a Example:
 * \verbatim
 int main (void)
 {
    char *remote_ip = NULL;
    uint16_t remote_port = (uint16_t)-1;

    #define TEST_PORT      (8080)
    xsock_clear_errno ();
    // Wait for incoming connection
    int fd = xsock_server (TEST_PORT);
    if (fd<=0) {
       fprintf (stderr, "Unable to establish server\n");
       return EXIT_FAILURE;
    }
    int confd = xsock_accept (fd, 10, &remote_ip, &remote_port);
    if (confd<=0) {
       fprintf (stderr, "No connection received on %u\n", TEST_PORT);
       fprintf (stderr, "err %i, %s\n", xsock_errno (),
                xsock_strerror(xsock_errno()));
       return EXIT_FAILURE;
    }

    fprintf (stderr, "Remote party is %s:%u\n", remote_ip, remote_port);
    free (remote_ip);

    // Read a string from the incoming connection
    char buffer[255]; buffer[255] = 0;
    size_t bytes_read = xsock_read (confd, buffer, sizeof buffer, 5);
    if (bytes_read==0) {
       printf ("Timeout\n");
    } else if (bytes_read<0) {
       printf ("Error reading: %s\n", xsock_strerror(xsock_errno()));
    } else {
      printf ("Read '%s'\n", buffer);
    }
    xsock_close (fd);
    xsock_close (confd);
 }
  \endverbatim
 */
int xsock_accept (int fd, size_t timeout, char **addr, uint16_t *port)
{
   struct sockaddr_in ret;
   socklen_t retlen = sizeof ret;
   int retval = -1;

   memset(&ret, 0xff, sizeof ret);

   fd_set fds;
   struct timeval tv = { timeout , 0 };
   FD_ZERO (&fds);
   FD_SET (fd, &fds);
   int r = select (fd + 1, &fds, &fds, &fds, &tv);
   if (r==0) {
      return 0;
   }
   retval = accept4 (fd, (struct sockaddr *)&ret, &retlen, SOCK_CLOEXEC);
   if (retval <= 0) {
      return -1;
   }

   /* This shoudld be performed by the caller on every accepted socket.
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
      *addr = malloc (16);
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

/**
 * \brief Connect to the specified address
 *
 * Connect to the specified address \a server on port \a port. The address
 * \a server can be either an IP address or a domain name. A single
 * connection attempt is made to connect to \a server on \a port only.
 *
 * \sa xsock_server(), xsock_accept(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_errno(), xsock_clear_errno(), xsock_strerror()
 *
 * @param[in] server The network address/domain to connect to
 * @param[in] port The port on which to connect
 *
 * @return On success a connected socket descriptor is returned which can
 *    be used with \a xsock_read(), \a xsock_write() and \a xsock_close(). The
 *    caller is responsible for closing the socket descriptor with
 *    \a xsock_close().  On error (size_t)-1 is returned and
 *   \a  xsock_errno() will return the specific error number.
 *
 * \a Example:
 * \verbatim
 int main (void)
 {
    #define TEST_PORT      (8080)
    xsock_clear_errno ();
    // Wait for incoming connection
    int fd = xsock_server (TEST_PORT);
    if (fd<=0) {
       fprintf (stderr, "Unable to establish server\n");
       return EXIT_FAILURE;
    }
    int confd = xsock_accept (fd, 10, NULL, NULL);
    if (confd<=0) {
       fprintf (stderr, "No connection received on %u\n", TEST_PORT);
       fprintf (stderr, "err %i, %s\n", xsock_errno (),
                xsock_strerror(xsock_errno()));
       return EXIT_FAILURE;
    }
    // Read a string from the incoming connection
    char buffer[255]; buffer[255] = 0;
    size_t bytes_read = xsock_read (confd, buffer, sizeof buffer, 5);
    if (bytes_read==0) {
       printf ("Timeout\n");
    } else if (bytes_read<0) {
       printf ("Error reading: %s\n", xsock_strerror(xsock_errno()));
    } else {
      printf ("Read '%s'\n", buffer);
    }
    xsock_close (fd);
    xsock_close (confd);
 }
  \endverbatim
 */
int xsock_connect (const char *server, size_t port)
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

/**
 * \brief Close a socket descriptor
 *
 * Closes the socket descriptor \a fd, which must have been returned from
 * a successful call to \a xsock_server(), xsock_accept() or \a xsock_close(). On error -1
 * is returned and the specific error number can be retrieved with
 * \a xsock_errno().
 *
 * \sa xsock_server(), xsock_accept(), xsock_connect(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_errno(), xsock_strerror()
 *
 * @return On success, zero is returned. On error, -1 is returned and the
 *    specific error number can be retrieved with \a xsock_errno().
 */
int xsock_close (int fd)
{
   /* ****************************************
    * 1. Call close() on the socket descriptor
    */
   SAFETY_CHECK;
   return close (fd);
}

/**
 * \brief Write data to a connected socket
 *
 * Write \a len bytes of data in buffer \a buf to socket descriptor \a fd.
 * The number of bytes actually written is returned. If an error occurs
 * (size_t)-1 is returned and the specific error number can be retrieved
 * with \a xsock_errno(). The write is performed in blocking mode.
 *
 * \sa xsock_server(), xsock_accept(), xsock_close(), xsock_connect(),
 *    xsock_read(), xsock_errno(), xsock_clear_errno(), xsock_strerror()
 *
 * @param[in] fd The socket descriptor to write to
 * @param[in] buf The data to be written
 * @param[in] len The number of bytes to write
 *
 * @return On success the number of bytes actually written to the socket
 *    descriptor is returned, or zero if no data is written.
 *    On error (size_t)-1 is returned and \a xsock_errno() will return the
 *    specific error number.
 *
 * \a Example:
 * \verbatim
 int main (void)
 {
    #define TEST_PORT      (8080)
    xsock_clear_errno ();
    // Wait for incoming connection
    int fd = xsock_server (TEST_PORT);
    if (fd<=0) {
       fprintf (stderr, "Unable to establish server\n");
       return EXIT_FAILURE;
    }
    int confd = xsock_accept (fd, 10, NULL, NULL);
    if (confd<=0) {
       fprintf (stderr, "No connection received on %u\n", TEST_PORT);
       fprintf (stderr, "err %i, %s\n", xsock_errno (),
                xsock_strerror(xsock_errno()));
       return EXIT_FAILURE;
    }
    // Read a string from the incoming connection
    char buffer[255]; buffer[255] = 0;
    size_t bytes_read = xsock_read (confd, buffer, sizeof buffer, 5);
    if (bytes_read==0) {
       printf ("Timeout\n");
    } else if (bytes_read<0) {
       printf ("Error reading: %s\n", xsock_strerror(xsock_errno()));
    } else {
      printf ("Read '%s'\n", buffer);
    }
    xsock_close (fd);
    xsock_close (confd);
 }
  \endverbatim
 */
size_t xsock_write (int fd, const void *buf, size_t len)
{
   SAFETY_CHECK;
   NETCODE_UTIL_LOG ("sending %i bytes\n", len);
   ssize_t retval = SEND (fd, buf, len);
   if (retval<0) return (size_t)-1;
   return retval;
}

/**
 * \brief Read data from given connected and live socket
 *
 * Read not more than \a len bytes into buffer \a buf from socket descriptor
 * \a fd over the next \a timeout seconds. Should the timeout expire
 * before the buffer is filled then the number of bytes read in is returned.
 * If no bytes are read in after timeout expires then zero is returned.
 * Should \a len bytes be read from the socket descriptor then the function
 * returns immediately without waiting for the timeout to expire.
 *
 * On failure -1 is returned and the specific error number can be retrieved
 * using \a xsock_errno().
 *
 * \sa xsock_server(), xsock_accept(), xsock_connect(), xsock_close(), xsock_write(),
 *    xsock_errno(), xsock_clear_errno(), xsock_strerror()
 *
 * @param[in] fd The socket descriptor to read
 * @param[in] buf The buffer to fill with data read in from \a fd
 * @param[in] len The length of the buffer \a buf in bytes
 * @param[in] timeout The timeout in seconds to wait for data to become
 *    available to read
 *
 * @return On success, the number of actual bytes read from the file
 *    descriptor \a fd is returned.  On error (size_t)-1 is returned and
 *    \a xsock_errno() will return the specific error number. On timeout
 *    the number of bytes read is returned, which may be zero if no
 *    bytes are read.
 *
 * \a Example:
 * \verbatim
 int main (void)
 {
    #define TEST_PORT      (8080)
    xsock_clear_errno ();
    // Wait for incoming connection
    int fd = xsock_server (TEST_PORT);
    if (fd<=0) {
       fprintf (stderr, "Unable to establish server\n");
       return EXIT_FAILURE;
    }
    int confd = xsock_accept (fd, 10, NULL, NULL);
    if (confd<=0) {
       fprintf (stderr, "No connection received on %u\n", TEST_PORT);
       fprintf (stderr, "err %i, %s\n", xsock_errno (),
                xsock_strerror(xsock_errno()));
       return EXIT_FAILURE;
    }
    // Read a string from the incoming connection
    char buffer[255]; buffer[255] = 0;
    size_t bytes_read = xsock_read (confd, buffer, sizeof buffer, 5);
    if (bytes_read==0) {
       printf ("Timeout\n");
    } else if (bytes_read<0) {
       printf ("Error reading: %s\n", xsock_strerror(xsock_errno()));
    } else {
      printf ("Read '%s'\n", buffer);
    }
    xsock_close (fd);
    xsock_close (confd);
 }
  \endverbatim
 */
size_t xsock_read (int fd, void *buf, size_t len, size_t timeout)
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
   NETCODE_UTIL_LOG ("Attempting to read %i bytes\n", len);
   do {
      fd_set fds;
      FD_ZERO (&fds);
      FD_SET (fd, &fds);
      int selresult = select (fd + 1, &fds, NULL, NULL, &tv);
      if (selresult>0) {
         xsock_clear_errno ();
         ssize_t r = recv (fd, &buffer[idx], len-idx, MSG_DONTWAIT);

         // Return error immediately if an error is detected. Reading zero
         // bytes from a socket that caused a select() to return means
         // that the other side has disconnected.
         if (xsock_errno ()) return idx ? idx : (size_t)-1;
         if (r == -1) return idx ? idx : (size_t)-1;
         if (r ==  0) return idx ? idx : (size_t)-1;

         idx += (size_t)r;
         NETCODE_UTIL_LOG ("read %i bytes\n", idx);
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

/**
 * \brief Clear the last error value
 *
 * Clears the last error that occurred. This function is not reentrant.
 *
 * \sa xsock_server(), xsock_accept(), xsock_connect(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_errno(), xsock_strerror()
 *
 * @return On success, zero is returned. On error, -1 is returned and the
 *    error is unspecified.
 */
int xsock_clear_errno (void)
{
   SAFETY_CHECK;
   errno = 0;
   return 0;
}

/**
 * \brief Retrieve the last error number
 *
 * Returns the number of the last error that occurred. This function is not
 * reentrant and will return the last error that occurred, even if the
 * occurrence was in a different thread.
 *
 * \sa xsock_server(), xsock_accept(), xsock_connect(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_clear_errno(), xsock_strerror()
 *
 * @return The value of the last error that occurred is returned.
 */
int xsock_errno (void)
{
   SAFETY_CHECK;
   return errno;
}

/**
 * \brief Turn the error number given into a human-readable string
 *
 * Changes the error number \a err into a human readable string that can
 * be displayed to the user. This function is not reentrant and the caller
 * should make a copy of the return value if the returned string is to be
 * used long after the call to this function is made.
 *
 * \sa xsock_server(), xsock_accept(), xsock_connect(), xsock_close(), xsock_write(),
 *    xsock_read(), xsock_clear_errno(), xsock_errno()
 *
 * @return A non-modifiable string describing the specified error \a err in
 *    a human-readable manner.
 */
const char *xsock_strerror (int err)
{
   SAFETY_CHECK;
   return strerror (err);
}

