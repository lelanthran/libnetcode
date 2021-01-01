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

   netcode_util_clear_errno ();

   printf ("SERVER-TCP: Listening on [%u] ... ", NETCODE_TEST_TCP_PORT);

   if ((listenfd = netcode_tcp_server (NETCODE_TEST_TCP_PORT))<0) {
      NETCODE_UTIL_LOG ("Failed to listen: [%i:%s].\n",
                         netcode_util_errno (),
                         netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   printf ("SERVER-TCP:  listening on fd [%i]\n", listenfd);

   printf ("SERVER-TCP: Waiting for connection (max %i seconds) ... ", TIMEOUT);
   if ((clientfd = netcode_tcp_accept (listenfd, TIMEOUT, &client_ip, &client_port))<0) {
      NETCODE_UTIL_LOG ("Timed out waiting for client to connect: [%i%s].\n",
                         netcode_util_errno (),
                         netcode_util_strerror (netcode_util_errno ()));
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
      netcode_util_close (listenfd);
   }
   if (clientfd >= 0) {
      netcode_util_close (clientfd);
   }
   return ret;
}

int udp_test (void)
{
   int ret = EXIT_FAILURE;
   uint8_t *rxdata = NULL;
   char *remote_ip = NULL;
   char *tmp = NULL;
   size_t rxlen = 0;
   size_t rc = 0;
   int udp_socket = -1;

   printf ("SERVER-UDP: Setting up datagram socket ... ");

   if ((udp_socket = netcode_udp_socket (NETCODE_TEST_UDP_SERVER_PORT, NULL)) < 0) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Failed to initialise socket, error %i [%s.\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }


   printf ("done.\nSERVER-UDP: Waiting for incoming datagram ... ");

   rc = netcode_udp_wait (udp_socket, &remote_ip, &rxdata, &rxlen, TIMEOUT);

   printf ("done\n");

   if (rc == (size_t)-1) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Error %i waiting for datagram: %s\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   if (rc == 0 && !*remote_ip) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Timed out waiting for datagram\n");
      goto errorexit;
   }

   printf ("SERVER-UDP: Received %zu/%zu bytes from [%s]\n",
           rc, rxlen, remote_ip);

   if ((memcmp (NETCODE_TEST_UDP_REQUEST, rxdata, rxlen))!=0) {
      if (!(tmp = calloc (1, rxlen + 1))) {
         NETCODE_UTIL_LOG ("OOM error\n");
         goto errorexit;
      }
      memcpy (tmp, rxdata, rxlen);

      NETCODE_UTIL_LOG ("SERVER-UDP: incorrect data rxed expected[%s], got[%s]\n",
                        NETCODE_TEST_UDP_REQUEST,
                        tmp);
      goto errorexit;
   }

   printf ("SERVER-UDP: Received [%s] from [%s]\n", rxdata, remote_ip);

   rc = netcode_udp_send (udp_socket, remote_ip, NETCODE_TEST_UDP_CLIENT_PORT,
                          (uint8_t *)NETCODE_TEST_UDP_RESPONSE,
                          strlen (NETCODE_TEST_UDP_RESPONSE) + 1, 
                          NULL);
   if (rc != (strlen (NETCODE_TEST_UDP_RESPONSE) + 1)) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Failed to transmit to [%s], udp_send() returned %zu\n",
                        remote_ip,
                        rc);
      goto errorexit;
   }

   printf ("SERVER-UDP: Responded with [%s]\n", NETCODE_TEST_UDP_RESPONSE);

   ret = EXIT_SUCCESS;

errorexit:
   free (tmp);
   free (rxdata);
   free (remote_ip);

   return ret;
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

   if (!(netcode_util_init ())) {
      NETCODE_UTIL_LOG ("SERVER: Failed to initialise netcode\n");
      goto errorexit;
   }

   for (size_t i=0; argv[1] && i<sizeof tests / sizeof tests[0]; i++) {
      if ((strcmp (tests[i].name, argv[1]))==0) {
         ret = tests[i].fptr ();
            NETCODE_UTIL_LOG ("SERVER [%s]: %s\n", tests[i].name, ret ? "failed" : "passed");
         goto errorexit;
      }
   }

   if ((ret = tcp_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ SERVER-TCP: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("***************************************\n");
   printf ("*** *** SERVER-TCP: Test passed *** ***\n");
   printf ("***************************************\n");

   if ((ret = udp_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ SERVER-UDP: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("***************************************\n");
   printf ("*** *** SERVER-UDP: Test passed *** ***\n");
   printf ("***************************************\n");

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}


