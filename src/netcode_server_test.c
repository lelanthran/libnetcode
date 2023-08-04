#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "netcode_util.h"
#include "netcode_tcp.h"
#include "netcode_udp.h"

#define TIMEOUT      (5)

static int tcp_test (void)
{
   int ret = EXIT_FAILURE;
   socket_t listenfd = -1, clientfd = -1;
   uint32_t rxlen = 8192;
   char *rx = malloc (rxlen);
   char *expected = NETCODE_TEST_TCP_REQUEST;
   uint32_t expected_len = (uint32_t)strlen (NETCODE_TEST_TCP_REQUEST);
   char *client_ip = NULL;
   uint16_t client_port;
   int64_t nbytes = 0;

   printf ("SERVER-TCP: Netcode server test.\n");

   if (!rx) {
      NETCODE_UTIL_LOG ("Failed to allocate %u bytes for response\n",
                        rxlen);
   }
   memset (rx, 0, rxlen);
   rxlen--;

   netcode_util_clear_errno ();

   printf ("SERVER-TCP: Listening on [%u] ... ", NETCODE_TEST_TCP_PORT);

   listenfd = netcode_tcp_server (NETCODE_TEST_TCP_PORT);
   if (!(NETCODE_SOCK_VALID(listenfd))) {
      NETCODE_UTIL_LOG ("Failed to listen: [%i:%s].\n",
                         netcode_util_errno (),
                         netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   printf ("SERVER-TCP:  listening on fd [%i]\n",(int)listenfd);

   printf ("SERVER-TCP: Waiting for connection (max %i seconds) ... ", TIMEOUT);
   clientfd = netcode_tcp_accept (listenfd, TIMEOUT, &client_ip, &client_port);
   if (!(NETCODE_SOCK_VALID(clientfd))) {
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
      NETCODE_UTIL_LOG ("Failed to receive %u bytes, got %" PRIi64 " instead.\n",
                         expected_len, nbytes);
      goto errorexit;
   }

   printf ("SERVER-TCP: received %" PRIi64 " bytes [%s].\n", nbytes, rx);

   if ((strcmp (rx, NETCODE_TEST_TCP_REQUEST))!=0) {
      NETCODE_UTIL_LOG ("Unexpected request: expected [%s], got [%s] instead.\n",
                         expected, rx);
   }

   const char *tx = NETCODE_TEST_TCP_RESPONSE;
   uint32_t txlen = (uint32_t)strlen (tx);

   if ((nbytes = netcode_tcp_write (clientfd, tx, txlen))!=txlen) {
      NETCODE_UTIL_LOG ("Failed to transmit %u bytes [%s], "
                        "transmitted %" PRIi64 " instead.\n",
                         rxlen, tx, nbytes);
      goto errorexit;
   }

   printf ("SERVER-TCP: Transmitted %u bytes [%s] ...\n", txlen, tx);
   printf ("SERVER-TCP: Waiting %i seconds to ensure that the socket is flushed\n", TIMEOUT);

   netcode_tcp_read (clientfd, rx, rxlen, TIMEOUT);

   ret = EXIT_SUCCESS;

errorexit:
   free (rx);
   free (client_ip);

   if ((NETCODE_SOCK_VALID (listenfd))) {
      netcode_util_close (listenfd);
   }
   if ((NETCODE_SOCK_VALID (clientfd))) {
      netcode_util_close (clientfd);
   }
   return ret;
}

int udp_test (void)
{
   int ret = EXIT_FAILURE;
   uint8_t *rxdata = NULL;
   char *remote_ip = NULL;
   uint16_t remote_port = 0;
   char *tmp = NULL;
   uint32_t rxlen = 0;
   int64_t rc = 0;
   socket_t udp_socket = -1;

   printf ("SERVER-UDP: Setting up datagram socket ... ");

   udp_socket = netcode_udp_socket (NETCODE_TEST_UDP_SERVER_PORT, NULL);
   if (!(NETCODE_SOCK_VALID(udp_socket))) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Failed to initialise socket, error %i [%s.\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }


   printf ("done.\nSERVER-UDP: Waiting for incoming datagram ... ");

   rc = netcode_udp_wait (udp_socket, &remote_ip, &remote_port, &rxdata, &rxlen, TIMEOUT);

   printf ("done\n");

   if (rc == (uint32_t)-1) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Error %i waiting for datagram: %s\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   if (rc == 0 && !remote_ip) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Timed out waiting for datagram\n");
      goto errorexit;
   }

   printf ("SERVER-UDP: Received %" PRIi64 "/%u bytes from [%s]\n",
           rc, rxlen, remote_ip);


   uint8_t *part2 = &rxdata[strlen (NETCODE_TEST_UDP_REQUEST1) + 1];
   if (((memcmp (NETCODE_TEST_UDP_REQUEST1, rxdata, strlen (NETCODE_TEST_UDP_REQUEST1)))!=0) ||
       ((memcmp (NETCODE_TEST_UDP_REQUEST2, part2, strlen (NETCODE_TEST_UDP_REQUEST2)))!=0)) {
      if (!(tmp = calloc (1, rxlen + 1))) {
         NETCODE_UTIL_LOG ("OOM error\n");
         goto errorexit;
      }
      memcpy (tmp, rxdata, rxlen);

      NETCODE_UTIL_LOG ("SERVER-UDP: incorrect data rxed expected[%s%s], got[%s]\n",
                        NETCODE_TEST_UDP_REQUEST1, NETCODE_TEST_UDP_REQUEST2,
                        tmp);
      goto errorexit;
   }

   printf ("SERVER-UDP: Received [%s%s] from [%s:%u]\n", rxdata, part2, remote_ip, remote_port);

   netcode_util_clear_errno ();
   rc = netcode_udp_send (udp_socket, remote_ip, NETCODE_TEST_UDP_CLIENT_PORT,
                          (uint8_t *)NETCODE_TEST_UDP_RESPONSE1,
                          (uint32_t)strlen (NETCODE_TEST_UDP_RESPONSE1) + 1,
                          (uint8_t *)NETCODE_TEST_UDP_RESPONSE2,
                          (uint32_t)strlen (NETCODE_TEST_UDP_RESPONSE2) + 1,
                          NULL);
   int errcode = netcode_util_errno ();
   const char *errmsg = netcode_util_strerror (errcode);
   if (rc != (strlen (NETCODE_TEST_UDP_RESPONSE1) +
               1 +
               strlen (NETCODE_TEST_UDP_RESPONSE2) + 1)) {
      NETCODE_UTIL_LOG ("SERVER-UDP: Failed to transmit to [%s], "
                        "udp_send() returned %" PRIi64 "\n"
                        "[%i:%s]\n",
                        remote_ip,
                        rc,
                        errcode, errmsg);
      goto errorexit;
   }

   printf ("SERVER-UDP: Responded with [%s%s]\n", NETCODE_TEST_UDP_RESPONSE1,
                                                  NETCODE_TEST_UDP_RESPONSE2);

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
         NETCODE_UTIL_LOG ("SERVER-%s[%s]: %s\n", tests[i].name, ret ? "failed" : "passed",
                                                   netcode_util_strerror (netcode_util_errno ()));
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


