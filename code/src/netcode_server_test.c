#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "netcode_util.h"
#include "netcode_tcp.h"
#include "netcode_udp.h"

int main (void)
{
   int ret = EXIT_FAILURE;
   int listenfd = -1, clientfd = -1;
   size_t rxlen = 8192;
   char *rx = malloc (rxlen - 1);
   char *expected = NETCODE_TEST_TCP_REQUEST;
   size_t expected_len = strlen (NETCODE_TEST_TCP_REQUEST);
   char *client_ip;
   uint16_t client_port;
   size_t nbytes = 0;

   printf ("Netcode server test.\n");

   if (!rx) {
      NETCODE_UTIL_LOG ("Failed to allocate %zu bytes for response\n",
                        rxlen);
   }
   memset (rx, 0, rxlen);

   netcode_tcp_clear_errno ();

   printf ("Listening on [%u] ... ", NETCODE_TEST_PORT);

   if ((listenfd = netcode_tcp_server (NETCODE_TEST_PORT))<0) {
      NETCODE_UTIL_LOG ("Failed to listen: [%i:%s].\n",
                         netcode_tcp_errno (),
                         netcode_tcp_strerror (netcode_tcp_errno ()));
      goto errorexit;
   }

   printf (" listening on fd [%i]\n", listenfd);

   printf ("Waiting for connection (max 5 seconds) ... ");
   if ((clientfd = netcode_tcp_accept (listenfd, 5, &client_ip, &client_port))<0) {
      NETCODE_UTIL_LOG ("Timed out waiting for client to connect: [%i%s].\n",
                         netcode_tcp_errno (),
                         netcode_tcp_strerror (netcode_tcp_errno ()));
      goto errorexit;
   }

   printf (" [%s:%u] connection received.\n", client_ip, client_port);

   printf ("Waiting for incoming data (max 5 seconds) ... ");

   if ((nbytes = netcode_tcp_read (clientfd, rx, rxlen, 5))!=expected_len) {
      NETCODE_UTIL_LOG ("Failed to receive %zu bytes, got %zu instead.\n",
                         expected_len, nbytes);
      goto errorexit;
   }

   printf ("received %zu bytes [%s].\n", nbytes, rx);

   if ((strcmp (rx, NETCODE_TEST_TCP_REQUEST))!=0) {
      NETCODE_UTIL_LOG ("Unexpected request: expected [%s], got [%s] instead.\n",
                         expected, rx);
   }

   const char *tx = NETCODE_TEST_TCP_RESPONSE;
   size_t txlen = strlen (tx);

   if ((nbytes = netcode_tcp_write (clientfd, tx, txlen))!=txlen) {
      NETCODE_UTIL_LOG ("Failed to transmit %zu bytes [%s], transmitted %zu instead.\n",
                         rxlen, tx, nbytes);
      goto errorexit;
   }

   printf ("Transmitted %zu bytes [%s] ...\n", txlen, tx);
   sleep (5);

   ret = EXIT_SUCCESS;

errorexit:
   free (rx);
   free (client_ip);

   if (listenfd >= 0) {
      netcode_tcp_close (listenfd);
   }
   if (clientfd >= 0) {
      netcode_tcp_close (clientfd);
   }
   return ret;
}


