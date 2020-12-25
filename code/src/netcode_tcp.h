
#ifndef H_NETCODE_TCP
#define H_NETCODE_TCP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

   /* Wait for a tcp connection on the specified port, waiting indefinitely.
    * The fd of the connected socket is returned.
    */
   int netcode_tcp_server (size_t port);

   /* Accept a client connection on a listening fd. If addr is not NULL, then
    * it is allocated and filled with the IP address of the remote peer. If
    * port is not NULL then is it filled with the port of the remote peer.
    */
   int netcode_tcp_accept (int fd, size_t timeout, char **addr, uint16_t *port);

   /* Make a connection to the specified server on the specified port.
    * The fd of the connected descriptor is returned.
    */
   int netcode_tcp_connect (const char *server, size_t port);

   /* Close the given file descriptor. fd must have been previously opened
    * using a netcode_tcp_*() function.
    */
   int netcode_tcp_close (int fd);

   /* Write the given buffer to the given fd, return number of bytes
    * successfully returned. On error (size_t)-1 is returned.
    */
   size_t netcode_tcp_write (int fd, const void *buf, size_t len);

   /* Read not more than the specified number of bytes from the given fd
    * into the specified buffer. On error (size_t) -1 is returned. No
    * more than timeout seconds is spent waiting for a message.
    */
   size_t netcode_tcp_read (int fd, void *buf, size_t len, size_t timeout);

   int netcode_tcp_clear_errno (void);
   int netcode_tcp_errno (void);
   const char *netcode_tcp_strerror (int err);

#ifdef __cplusplus
};
#endif


#endif


