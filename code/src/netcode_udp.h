
#ifndef H_NETCODE_UDP
#define H_NETCODE_UDP

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

   // Returns a datagram socket that, on receiving, will receive on
   // the port specified and, on sending, will send to the host
   // specified on the port specified.
   //
   // On sending, a different host may be specified.
   // On error will return -1 and the error and error message can be
   // retrieved using the _errno() and _strerror() functions.
   int netcode_udp_socket (uint16_t port, const char *host);

   // Will wait not less than 'timeout' seconds for a datagram
   // on the port that the socket fd is connected on. When a datagram
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
   // On error (size_t)-1 will be returned. On timeout zero will be
   // returned and '*buflen ' will be set to zero, '*remote_host'
   // will be set to NULL and the '*buf" will be set to NULL.
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
   size_t netcode_udp_wait (int fd, char **remote_host,
                            uint8_t **buf, size_t *buflen,
                            size_t timeout);

   // Will send the data in 'buf' on the datagram socket 'fd'. If the
   // parameter 'remote_host' is not NULL, then the datagram will be
   // sent to the host specified in 'remote_host'.
   //
   // If remote_host is NULL then the datagram will be sent to the host
   // specified in the netcode_udp_socket() call that was used to create
   // the 'fd'.
   size_t netcode_udp_send (int fd, char *remote_host,
                            uint8_t *buf, size_t buflen);

   // Returns the last error number that occurred.
   int netcode_udp_errno (void);

   // Returns a human-readable string describing the error specified in
   // the parameter 'error'.
   const char *netcode_udp_strerror (int error);

#ifdef __cplusplus
};
#endif


#endif


