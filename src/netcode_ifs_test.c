#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "netcode_util.h"
#include "netcode_ifs.h"

static int ifs_test (void)
{
   int ret = EXIT_FAILURE;

   netcode_ifs_t **list = netcode_ifs_list_new ();

   if (!list) {
      NETCODE_UTIL_LOG ("Failed to get list of interfaces\n");
      goto errorexit;
   }

   for (size_t i=0; list[i]; i++) {
   bool netcode_ifs_extract (const netcode_ifs_t *ifs,
                             uint64_t  *dst_if_flags,
                             char **dst_if_name,
                             char **dst_if_addr,
                             char **dst_if_netmask,
                             char **dst_if_broadcast,
                             char **dst_if_p2paddr);
   }

   ret = EXIT_SUCCESS;
errorexit:

   netcode_ifs_list_del (flist);

   return ret;
}

int main (void)
{
   int ret = EXIT_FAILURE;

   if ((ret = ifs_test ())!=EXIT_SUCCESS) {
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      printf ("+++ +++ IFS: Test FAILED +++ +++\n");
      printf ("+++++++++++++++++++++++++++++++++++++++\n");
      goto errorexit;
   }

   printf ("********************************\n");
   printf ("*** *** IFS: Test passed *** ***\n");
   printf ("********************************\n");

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

