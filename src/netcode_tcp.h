
#ifndef H_NETCODE_TCP
#define H_NETCODE_TCP

#include <stddef.h>
#include <stdint.h>

#include "netcode_util.h"

#ifdef __cplusplus
extern "C" {
#endif

   /* Set up a listening socket on the specified port. On success
    * the fd of the connected socket is returned. On error -1 is returned.
    */
   socket_t netcode_tcp_server (uint16_t port);

   /* Accept a client connection on a listening fd. If addr is not NULL, then
    * it is allocated and filled with the IP address of the remote peer. If
    * port is not NULL then is it filled with the port of the remote peer.
    *
    * On error -1 is returned. On timeout zero is returned. On success a non-zero
    * file descriptor is returned.
    */
   socket_t netcode_tcp_accept (socket_t fd, uint32_t timeout_secs, char **addr, uint16_t *port);

   /* Make a connection to the specified server on the specified port.
    * On success the fd of the connected descriptor is returned. On error -1
    * is returned.
    */
   socket_t netcode_tcp_connect (const char *server, uint16_t port);

   /* Write the given buffer to the given fd. On success the number
    * of bytes written is returned, which may be less than the specified
    * number of bytes.
    *
    * On error (uint32_t)-1 is returned.
    */
   uint32_t netcode_tcp_write (socket_t fd, const void *buf, uint32_t len);

   /* Read not more than the specified number of bytes from the given fd
    * into the specified buffer. This function returns when the specified
    * buffer is full or when the timeout expires, whichever happens first.
    *
    * On success the number of bytes that were actually read is returned,
    * which will be less than or equal to the length specified. On error
    * (uint32_t) -1 is returned.
    *
    * No more than timeout seconds is spent filling the buffer.
    */
   uint32_t netcode_tcp_read (socket_t fd, void *buf, uint32_t len, uint32_t timeout);

#ifdef __cplusplus
};
#endif


#endif


