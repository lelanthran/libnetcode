
#ifndef H_NETCODE_UDPS
#define H_NETCODE_UDPS

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct netcode_udps_t netcode_udps_t;

#ifdef __cplusplus
extern "C" {
#endif

   // Returns a handle to a udps data object. The data object will be
   // unconfigured and will still need to be configured using _udps_open().
   //
   // On a memory allocation failure NULL is returned, otherwise a pointer
   // to the data object is returned. No failures other than out of memory
   // errors can occur in this function.
   netcode_udps_t *netcode_udps_new (void);

   // Delete a udps data object, closing it first if necessary.
   void netcode_udps_del (netcode_udps_t *handle);

   // Sets up a socket as both a datagram listener and a datagram sender
   // to the specified host on the specified port. All transmissions will
   // be only to the specified host on the specified port. Reception will
   // be from any host on the specified port.
   int netcode_udps_open (netcode_udps_t *handle, const char *host, uint16_t port);

   // Closes and shuts down the socket opened in netcode_udps_open(). The
   // handle is still valid and can be reused by calling netcode_udps_open()
   // again.
   //
   // To delete the handle and free up all resources associated with it use
   // netcode_udps_del(), which will also close any open sockets.
   void netcode_udps_close (netcode_udps_t *handle);

   // Returns the last error and the last error message.
   int netcode_udps_last_error_code (netcode_udps_t *handle);
   const char *netcode_udps_last_error_message (netcode_udps_t *handle);

   // Configure some of the knobs on the data object:

   // MTU cannot be shrunk while there is still a pending transmission.
   // Note that the MTU is only a hint and may be ignored if network
   // conditions or peer negotiation determines that the specified MTU
   // is not suitable.
   //
   // A small value for MTU might result in incurring unnecessary overhead
   // as too high a percentage of the transmitted data will be datagram and
   // IP headers. An excessively large value for the MTU would result in
   // a large amount of memory being reserved.
   //
   // Reserved memory = MTU x NumberOfWindows.
   bool netcode_udps_set_mtu (netcode_udps_t *handle, size_t mtu_in_bytes);

   // Window size is clamped to the range of 1..32 inclusive. Too few
   // windows in a transmission can result in an unrecoverably broken
   // transmission. Too many windows can result in excessively large amounts
   // of memory being reserved.
   //
   // Reserved memory = MTU x NumberOfWindows.
   void netcode_udps_set_window_size (netcode_udps_t *handle, uint8_t wsize);

   // Add data to be transmitted. When called with NULL as the buffer, the
   // data added up to that point will be transmitted as a single datagram.
   // When the buffer is full it is automatically transmitted and any data
   // that did not fit will be pt into the next pending transmission.
   //
   // On success returns zero. On error returns the errorcode that can be
   // displayed with netcode_udp_strerror().
   int netcode_udps_send (netcode_udps_t *handle, void *buf, size_t len);

   // Return the next datagram that arrived, waiting not more than timeout
   // seconds. On timeout, zero is returned. On error, (size_t)-1 is returned
   // and the specific error can be retrieved with _get_last_error_code() and
   // _get_last_error_message().
   //
   // On success the number of bytes received is returned and copied into
   // *len. *buf is allocated as necessary. If *remote_host is not NULL,
   // remote_host is allocated and populated with the IP address of the
   // remote host.
   //
   // Both *buf and *remote_host must be freed by the caller.
   size_t netcode_udps_wait (netcode_udps_t *handle, void **buf, size_t *len,
                                                     size_t timeout_secs,
                                                     char **remote_host);

#ifdef __cplusplus
};
#endif


#endif
