#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "netcode_util.h"
#include "netcode_if.h"

/* ***************************************************************** */
static char *lstrdup (const char *src)
{
   char *ret = malloc (strlen (src) + 1);
   if (ret)
      strcpy (ret, src);
   return ret;
}

/* ***************************************************************** */
struct netcode_if_t {
   uint64_t   if_flags;
   char      *if_name;
   char      *if_addr;
   char      *if_netmask;
   char      *if_broadcast;
   char      *if_p2paddr;
};

static void netcode_if_del (netcode_if_t *interface)
{
   if (!interface)
      return;

   free (interface->if_name);
   free (interface->if_addr);
   free (interface->if_netmask);
   free (interface->if_broadcast);
   free (interface->if_p2paddr);

   free (interface);
}

static netcode_if_t *netcode_if_new (uint64_t if_flags,
                                     const char *if_name,
                                     const char *if_addr,
                                     const char *if_netmask,
                                     const char *if_broadcast,
                                     const char *if_p2paddr)
{
   netcode_if_t *ret = calloc (1, sizeof *ret);
   if (!ret)
      return NULL;

   ret->if_flags     = if_flags;
   ret->if_name      = lstrdup (if_name);
   ret->if_addr      = lstrdup (if_addr);
   ret->if_netmask   = lstrdup (if_netmask);
   ret->if_broadcast = lstrdup (if_broadcast);
   ret->if_p2paddr   = lstrdup (if_p2paddr);

   if (!ret->if_name      ||
       !ret->if_addr      ||
       !ret->if_netmask   ||
       !ret->if_broadcast ||
       !ret->if_p2paddr) {

      netcode_if_del (ret);
      ret = NULL;
   }
   return ret;
}

/* ***************************************************************** */

#if 0
   netcode_if_t **netcode_if_list_new (void);
   void netcode_if_list_del (netcode_if_t **list);

   bool netcode_if_extract (const netcode_if_t *interface,
                            uint64_t   *dst_if_flags,
                            char      **dst_if_name,
                            char      **dst_if_addr,
                            char      **dst_if_netmask,
                            char      **dst_if_broadcast,
                            char      **dst_if_p2paddr);
#endif

/* ***************************************************************** */
#if defined (OSTYPE_Darwin)
// .................
#endif

/* ***************************************************************** */
#ifdef PLATFORM_Windows
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>

// ...................

#endif

/* ***************************************************************** */
#ifdef PLATFORM_POSIX

netcode_if_t **netcode_if_list_new (void)
{
   return NULL;
}

void netcode_if_list_del (netcode_if_t **list)
{
   for (size_t i=0; list && list[i]; i++) {
      netcode_if_del (list[i]);
   }
   free (list);
}

bool netcode_if_extract (const netcode_if_t *interface,
                         uint64_t   *dst_if_flags,
                         char      **dst_if_name,
                         char      **dst_if_addr,
                         char      **dst_if_netmask,
                         char      **dst_if_broadcast,
                         char      **dst_if_p2paddr)
{
   return false;
}


#endif


