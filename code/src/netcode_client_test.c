#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "netcode_util.h"
#include "netcode_tcp.h"
#include "netcode_udp.h"

#define TIMEOUT         (10)

static int tcp_test (void)
{
   int ret = EXIT_FAILURE;
   int fd = -1;
   size_t rxlen = 8192;
   char *rx = malloc (rxlen);
   char *expected = NETCODE_TEST_TCP_RESPONSE;
   size_t expected_len = strlen (NETCODE_TEST_TCP_RESPONSE);

   printf ("CLIENT-TCP: Netcode client test.\n");

   if (!rx) {
      NETCODE_UTIL_LOG ("Failed to allocate %zu bytes for response\n",
                        rxlen);
   }
   memset (rx, 0, rxlen);
   rxlen--;

   netcode_tcp_clear_errno ();

   printf ("CLIENT-TCP: Connecting to [%s:%u] ... ", NETCODE_TEST_SERVER, NETCODE_TEST_PORT);

   if ((fd = netcode_tcp_connect (NETCODE_TEST_SERVER, NETCODE_TEST_PORT))==-1) {
      NETCODE_UTIL_LOG ("Failed to connect: [%i:%s].\n",
                         netcode_tcp_errno (),
                         netcode_tcp_strerror (netcode_tcp_errno ()));
      goto errorexit;
   }

   printf ("CLIENT-TCP:  connected [%i]\n", fd);

   const char *tx = NETCODE_TEST_TCP_REQUEST;
   size_t txlen = strlen (tx);
   size_t nbytes = 0;

   if ((nbytes = netcode_tcp_write (fd, tx, txlen))!=txlen) {
      NETCODE_UTIL_LOG ("Failed to transmit %zu bytes [%s], transmitted %zu instead.\n",
                         rxlen, tx, nbytes);
      goto errorexit;
   }

   printf ("CLIENT-TCP: Transmitted %zu bytes [%s] ...\n", txlen, tx);

   if ((nbytes = netcode_tcp_read (fd, rx, rxlen, TIMEOUT))!=expected_len) {
      NETCODE_UTIL_LOG ("Failed to receive %zu bytes, got %zu instead.\n",
                         expected_len, nbytes);
      goto errorexit;
   }

   printf ("CLIENT-TCP: Received %zu bytes [%s].\n", nbytes, rx);

   if ((strcmp (rx, NETCODE_TEST_TCP_RESPONSE))!=0) {
      NETCODE_UTIL_LOG ("Unexpected response: expected [%s], got [%s] instead.\n",
                         expected, rx);
   }

   ret = EXIT_SUCCESS;

errorexit:
   free (rx);
   if (fd >= 0) {
      netcode_tcp_close (fd);
   }
   return ret;
}

int main (void)
{
   int ret = EXIT_FAILURE;

   if ((ret = tcp_test ())!=EXIT_SUCCESS) {
      NETCODE_UTIL_LOG ("CLIENT-TCP: Test failed\n");
      goto errorexit;
   }

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

