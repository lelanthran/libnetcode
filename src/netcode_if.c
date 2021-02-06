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


/* ***************************************************************** */
/* ***************************************************************** */
#if defined (OSTYPE_Darwin)
// .................
#endif

/* ***************************************************************** */
/* ***************************************************************** */
#ifdef PLATFORM_Windows
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>

// ...................

#endif

/* ***************************************************************** */
/* ***************************************************************** */
#ifdef PLATFORM_POSIX

#include <sys/types.h>
#include <ifaddrs.h>

netcode_if_t **netcode_if_list_new (void)
{
   bool error = true;

   netcode_if_t **ret = NULL;
   struct ifaddrs *if_head = NULL,
                  *if_tmp = NULL;
   size_t nelems = 0;

   char *laddr       = NULL,
        *lnetmask    = NULL,
        *lbroadcast  = NULL,
        *lp2p        = NULL;

   if ((getifaddrs (&if_head))!=0) {
      // TODO: We need to record the error here.
      goto errorexit;
   }

   for (if_tmp = if_head; if_tmp != NULL; if_tmp = if_tmp->ifa_next)
      nelems++;

   if (!(ret = calloc (nelems + 1, sizeof *ret))) {
      // TODO: Record the error here
      goto errorexit;
   }

   size_t idx = 0;
   for (if_tmp = if_head; if_tmp != NULL; if_tmp = if_tmp->ifa_next) {

      free (laddr);        laddr       = NULL;
      free (lnetmask);     lnetmask    = NULL;
      free (lbroadcast);   lbroadcast  = NULL;
      free (lp2p);         lp2p        = NULL;

      uint64_t lflags = if_tmp->ifa_flags;
      const char *lname = if_tmp->ifa_name;

      // TODO: Stopped here last, must convert sock_addr to strings, and replace the
      // default string literals below with the empty string (so that we can still free
      // them the next time we loop);
      if (!laddr)      laddr      = lstrdup ("default-addr");
      if (!lnetmask)   lnetmask   = lstrdup ("default-netmask");
      if (!lbroadcast) lbroadcast = lstrdup ("default-broadcast");
      if (!lp2p)       lp2p       = lstrdup ("default-p2paddr");

      if (!laddr || !lnetmask ||! lbroadcast || !lp2p) {
         // TODO: Record error here
         goto errorexit;
      }

#if 0
      ifr->if_flags = if_tmp->ifa_flags;
      ifr->if_name      = lstrdup (if_name);
      ifr->if_addr      = lstrdup (if_addr);
      ifr->if_netmask   = lstrdup (if_netmask);
      ifr->if_broadcast = lstrdup (if_broadcast);
      ifr->if_p2paddr   = lstrdup (if_p2paddr);
#endif

      if (!(ret[idx++] = netcode_if_new (lflags, lname, laddr, lnetmask, lbroadcast, lp2p))) {
         // TODO: Record the error here
         goto errorexit;
      }
   }

   error = false;

errorexit:
   if (if_head)
      freeifaddrs (if_head);

   free (laddr);
   free (lnetmask);
   free (lbroadcast);
   free (lp2p);

   if (error) {
      netcode_if_list_del (ret);
      ret = NULL;
   }

   return ret;
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
   bool error = true;

   if (!interface)
      return false;

   if (dst_if_flags)
      *dst_if_flags = interface->if_flags;

#define CONDITIONAL_STRCPY(dst,src)      do {\
   if (dst) {\
      if (((*dst) = lstrdup (src))==NULL) {\
         goto errorexit;\
      }\
   }\
} while (0)

   CONDITIONAL_STRCPY (dst_if_name,       interface->if_name);
   CONDITIONAL_STRCPY (dst_if_addr,       interface->if_addr);
   CONDITIONAL_STRCPY (dst_if_netmask,    interface->if_netmask);
   CONDITIONAL_STRCPY (dst_if_broadcast,  interface->if_broadcast);
   CONDITIONAL_STRCPY (dst_if_p2paddr,    interface->if_p2paddr);

#undef CONDITIONAL_STRCPY

   error = false;
errorexit:
   return true;
}


#endif


