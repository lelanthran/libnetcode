
#ifndef H_NETCODE_UDP
#define H_NETCODE_UDP

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "netcode_util.h"

#ifdef __cplusplus
extern "C" {
#endif

   // Returns a datagram socket that, on receiving, will receive on
   // the port specified and, on sending, will send to the host
   // specified on the port specified.
   //
   // The port number is mandatory. The default_host is optional and
   // can be NULL. If no host is specified _udp_send() will fail unless
   // a host is provided.
   //
   // On sending, a different host may be specified.
   //
   // The caller must check if the returned socket is valid using the
   // macro NETCODE_SOCK_VALID(fd) which evaluates to true if the socket
   // is valid and false otherwise.
   //
   // On error the error value and message can be retrieved using the
   // _errno() and _strerror() functions.
   socket_t netcode_udp_socket (uint16_t listen_port, const char *default_host);

   // Will wait not less than 'timeout' seconds for a datagram
   // on the port that the socket is connected on. When a datagram
   // is received the peer's IP address will be copied into the
   // parameter 'remote_host' if 'remote_host' is not NULL. The
   // caller must free 'remote_host'.
   //
   // When 'timeout' is zero this function will check for a datagram
   // and return immediately. If there was no datagram then no data
   // will be returned (see the notes below for timeout conditions).
   // If a datagram is available it will be returned immediately.
   //
   // The buffer 'buf' will be allocated and filled with the entire
   // datagram. The caller must free 'buf'.
   //
   // The size of the datagram will be copied into 'buflen' and
   // will also be returned.
   //
   // On error -1 will be returned. On timeout zero will be
   // returned and '*buflen ' will be set to zero, '*remote_host'
   // will be set to NULL and the '*buf' will be set to NULL.
   //
   // On receipt of an empty datagram, zero will be returned and
   // *buflen will be set to zero, the '*remote_host' will be set
   // to the address of the remote_host and '*buf' will be set to
   // NULL.
   //
   // On receipt of a non-empty datagram, the length will be returned,
   // the length will be copied into '*buflen', the data will be copied
   // into an allocated buffer in '*buf' which the caller must free and
   // the remote host's ip address will be copied into '*remote_host'
   // which the caller must free.
   //
   int64_t netcode_udp_wait (socket_t fd, char **remote_host, uint16_t *remote_port,
                             uint8_t **buf, uint32_t *buflen,
                             uint32_t timeout);

   // Will send the data in the buffers specified on the datagram socket
   // 'fd'. If the parameter 'remote_host' is not NULL, then the datagram
   // will be sent to the host specified in 'remote_host'.
   //
   // The buffers are all packaged into a single datagram in the order
   // they are specified with no padding between the buffers. Each buffer
   // is specified as a tuple of {pointer, length}.
   //
   // If remote_host is NULL then the datagram will be sent to the host
   // specified in the netcode_udp_socket() call that was used to create
   // the 'fd'.
   //
   // RETURNS: -1 on error, the number of bytes transmitted on success.
   // The variants differ only in how the buffers are specified:
   //    senda() takes an array of buffer pointers and an array
   //       of buffer lengths. buf_array[i] will have buf_length[i].
   //    send() takes { buffer, buffer_length } parameters, repeated
   //       for each buffer, terminated with a NULL pointer.
   //    sendv() takes { buffer, buffer_length } parameters, repeated
   //       for each buffer, terminated with a NULL pointer, using the
   //       va_list pointer instead of literal parameters.
   int64_t netcode_udp_senda (socket_t fd, const char *remote_host, uint16_t port,
                              uint32_t nbuffers,
                              const void **buffers, uint32_t *buffer_lengths);

   int64_t netcode_udp_send (socket_t fd, const char *remote_host, uint16_t port,
                             const void *buf1, uint32_t buflen1,
                             ...);

   int64_t netcode_udp_sendv (socket_t fd, const char *remote_host, uint16_t port,
                              const void *buf1, uint32_t buflen1,
                              va_list ap);

#ifdef __cplusplus
};
#endif


#endif


