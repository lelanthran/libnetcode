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

   netcode_util_clear_errno ();

   // printf ("CLIENT-TCP: Connecting to [%s:%u] ... ", "example.noname", NETCODE_TEST_TCP_PORT);
   printf ("CLIENT-TCP: Connecting to [%s:%u] ... ", NETCODE_TEST_SERVER, NETCODE_TEST_TCP_PORT);

   // if ((fd = netcode_tcp_connect ("example.noname", NETCODE_TEST_TCP_PORT))==-1) {
   if ((fd = netcode_tcp_connect (NETCODE_TEST_SERVER, NETCODE_TEST_TCP_PORT))==-1) {
      NETCODE_UTIL_LOG ("Failed to connect: [%i:%s].\n",
                         netcode_util_errno (),
                         netcode_util_strerror (netcode_util_errno ()));
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
      netcode_util_close (fd);
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

   printf ("CLIENT-UDP: Setting up datagram socket ... ");

   if ((udp_socket = netcode_udp_socket (NETCODE_TEST_UDP_CLIENT_PORT, NULL)) < 0) {
      NETCODE_UTIL_LOG ("CLIENT-UDP: Failed to initialise socket, error %i [%s.\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   printf ("done\nCLIENT-UDP: sending datagram to [%s] ... ", NETCODE_TEST_SERVER);

   rc = netcode_udp_send (udp_socket, NETCODE_TEST_SERVER, NETCODE_TEST_UDP_SERVER_PORT,
                          (uint8_t *)NETCODE_TEST_UDP_REQUEST1,
                          strlen (NETCODE_TEST_UDP_REQUEST1) + 1,
                          (uint8_t *)NETCODE_TEST_UDP_REQUEST2,
                          strlen (NETCODE_TEST_UDP_REQUEST2) + 1,
                          NULL);

   if (rc != (strlen (NETCODE_TEST_UDP_REQUEST1) + 1 + strlen (NETCODE_TEST_UDP_REQUEST2) + 1)) {
      NETCODE_UTIL_LOG ("CLIENT-UDP: Error %i to transmit to [%s]: %s\n",
                        netcode_util_errno (),
                        remote_ip,
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   printf ("done\n");

   printf ("CLIENT-UDP: Waiting for response datagram ... ");

   rc = netcode_udp_wait (udp_socket, &remote_ip, &rxdata, &rxlen, TIMEOUT);

   printf ("done\n");

   if (rc == (size_t)-1) {
      NETCODE_UTIL_LOG ("CLIENT-UDP: Error %i waiting for datagram: %s\n",
                        netcode_util_errno (),
                        netcode_util_strerror (netcode_util_errno ()));
      goto errorexit;
   }

   if (rc == 0) {
      NETCODE_UTIL_LOG ("CLIENT-UDP: Timed out waiting for datagram\n");
      goto errorexit;
   }

   printf ("CLIENT-UDP: Received %zu/%zu bytes from [%s]\n",
           rc, rxlen, remote_ip);

   uint8_t *part2 = &rxdata[strlen (NETCODE_TEST_UDP_RESPONSE1) + 1];
   if (((memcmp (NETCODE_TEST_UDP_RESPONSE1, rxdata, strlen (NETCODE_TEST_UDP_RESPONSE1)))!=0) ||
       ((memcmp (NETCODE_TEST_UDP_RESPONSE2, part2, strlen (NETCODE_TEST_UDP_RESPONSE2)))!=0)) {
      if (!(tmp = calloc (1, rxlen + 1))) {
         NETCODE_UTIL_LOG ("OOM error\n");
         goto errorexit;
      }
      memcpy (tmp, rxdata, rxlen);

      NETCODE_UTIL_LOG ("CLIENT-UDP: incorrect data rxed [%s]\n",
                        tmp);
      goto errorexit;
   }

   printf ("CLIENT-UDP: Received [%s%s]\n", rxdata, part2);

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
         NETCODE_UTIL_LOG ("CLIENT-%s[%s]: %s\n", tests[i].name, ret ? "failed" : "passed",
                                                  netcode_util_strerror (netcode_util_errno ()));
         goto errorexit;
      }
   }

   if ((ret = tcp_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ CLIENT-TCP: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("***************************************\n");
   printf ("*** *** CLIENT-TCP: Test passed *** ***\n");
   printf ("***************************************\n");

   if ((ret = udp_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ CLIENT-UDP: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("***************************************\n");
   printf ("*** *** CLIENT-UDP: Test passed *** ***\n");
   printf ("***************************************\n");

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

