#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "netcode_util.h"
#include "netcode_ifs.h"

static int ifs_test (void)
{
   return EXIT_FAILURE;
}

int main (int argc, char **argv)
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

