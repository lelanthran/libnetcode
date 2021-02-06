#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "netcode_util.h"
#include "netcode_if.h"

static int if_test (void)
{
   int ret = EXIT_FAILURE;

   uint64_t if_flags = 0;
   char *if_name = NULL,
        *if_addr = NULL,
        *if_netmask = NULL,
        *if_broadcast = NULL,
        *if_p2paddr = NULL;

   netcode_if_t **list = netcode_if_list_new ();

   if (!list) {
      NETCODE_UTIL_LOG ("Failed to get list of interfaces\n");
      goto errorexit;
   }

   size_t ninterfaces = 0;
   for (size_t i=0; list[i]; i++) {
      ninterfaces++;
   }

   NETCODE_UTIL_LOG ("Found %zu interfaces\n", ninterfaces);

   for (size_t i=0; list[i]; i++) {

      if_flags = 0;

      free (if_name);      if_name      = NULL;
      free (if_addr);      if_addr      = NULL;
      free (if_netmask);   if_netmask   = NULL;
      free (if_broadcast); if_broadcast = NULL;
      free (if_p2paddr);   if_p2paddr   = NULL;

      const netcode_if_t *interface = list[i];
      bool rc = netcode_if_extract (interface,
                                    &if_flags,
                                    &if_name,
                                    &if_addr,
                                    &if_netmask,
                                    &if_broadcast,
                                    &if_p2paddr);
      if (!rc) {
         NETCODE_UTIL_LOG ("Failed to get information for interface %zu\n", i);
      }
      NETCODE_UTIL_LOG ("Information for interface [%zu]\n", i);
      NETCODE_UTIL_LOG ("     interface [%s:0x%08" PRIx64 "]\n", if_name, if_flags);
      NETCODE_UTIL_LOG ("     addr      [%s]\n", if_addr);
      NETCODE_UTIL_LOG ("     netmask   [%s]\n", if_netmask);
      NETCODE_UTIL_LOG ("     broadcast [%s]\n", if_broadcast);
      NETCODE_UTIL_LOG ("     p2paddr   [%s]\n", if_p2paddr);
   }

   ret = EXIT_SUCCESS;

errorexit:

   free (if_name);
   free (if_addr);
   free (if_netmask);
   free (if_broadcast);
   free (if_p2paddr);

   netcode_if_list_del (list);

   return ret;
}

int main (void)
{
   int ret = EXIT_FAILURE;

   if ((ret = if_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ if: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("********************************\n");
   printf ("*** *** if: Test passed *** ***\n");
   printf ("********************************\n");

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

