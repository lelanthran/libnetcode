#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "netcode_util.h"
#include "netcode_tcp.h"
#include "netcode_udp.h"

#define TIMEOUT      (5)

static int tcp_test (void)
{
   int ret = EXIT_FAILURE;
   int listenfd = -1, clientfd = -1;
   size_t rxlen = 8192;
   char *rx = malloc (rxlen);
   char *expected = NETCODE_TEST_TCP_REQUEST;
   size_t expected_len = strlen (NETCODE_TEST_TCP_REQUEST);
   char *client_ip = NULL;
   uint16_t client_port;
   size_t nbytes = 0;

   printf ("SERVER-TCP: Netcode server test.\n");

   if (!rx) {
      NETCODE_UTIL_LOG ("Failed to allocate %zu bytes for response\n",
                        rxlen);
   }
   memset (rx, 0, rxlen);
   rxlen--;

   netcode_tcp_clear_errno ();

   printf ("SERVER-TCP: Listening on [%u] ... ", NETCODE_TEST_PORT);

   if ((listenfd = netcode_tcp_server (NETCODE_TEST_PORT))<0) {
      NETCODE_UTIL_LOG ("Failed to listen: [%i:%s].\n",
                         netcode_tcp_errno (),
                         netcode_tcp_strerror (netcode_tcp_errno ()));
      goto errorexit;
   }

   printf ("SERVER-TCP:  listening on fd [%i]\n", listenfd);

   printf ("SERVER-TCP: Waiting for connection (max %i seconds) ... ", TIMEOUT);
   if ((clientfd = netcode_tcp_accept (listenfd, TIMEOUT, &client_ip, &client_port))<0) {
      NETCODE_UTIL_LOG ("Timed out waiting for client to connect: [%i%s].\n",
                         netcode_tcp_errno (),
                         netcode_tcp_strerror (netcode_tcp_errno ()));
      goto errorexit;
   }

   if (clientfd == 0) {
      NETCODE_UTIL_LOG ("Timed out waiting for client\n");
      goto errorexit;
   }

   printf ("SERVER-TCP:  [%s:%u] connection received.\n", client_ip, client_port);

   printf ("SERVER-TCP: Waiting for incoming data (max %i seconds) ... ", TIMEOUT);

   if ((nbytes = netcode_tcp_read (clientfd, rx, rxlen, TIMEOUT))!=expected_len) {
      NETCODE_UTIL_LOG ("Failed to receive %zu bytes, got %zu instead.\n",
                         expected_len, nbytes);
      goto errorexit;
   }

   printf ("SERVER-TCP: received %zu bytes [%s].\n", nbytes, rx);

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

   printf ("SERVER-TCP: Transmitted %zu bytes [%s] ...\n", txlen, tx);
   printf ("SERVER-TCP: Waiting %i seconds to ensure that the socket is flushed\n", TIMEOUT);

   netcode_tcp_read (clientfd, rx, rxlen, TIMEOUT);

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

int udp_test (void)
{
   return EXIT_FAILURE;
}

int main (int argc, char **argv)
{
   int ret = EXIT_FAILURE;
   static const struct {
      const char *name;
      int (*fptr) (void);
   } tests [] = {
      { "tcp_test", tcp_test },
      { "udp_test", udp_test },
   };

   (void) argc;

   for (size_t i=0; argv[1] && i<sizeof tests / sizeof tests[0]; i++) {
      if ((strcmp (tests[i].name, argv[1]))==0) {
         ret = tests[i].fptr ();
         goto errorexit;
      }
   }

   if ((ret = tcp_test ())!=EXIT_SUCCESS) {
      NETCODE_UTIL_LOG ("SERVER-TCP: Test failed\n");
      goto errorexit;
   }

   if ((ret = udp_test ())!=EXIT_SUCCESS) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Test failed\n");
      goto errorexit;
   }

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}


