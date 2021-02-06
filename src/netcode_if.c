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
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <ifaddrs.h>

static uint64_t if_reflag (uint64_t flags)
{
   uint64_t ret = 0;

#define CHECKSET(checkflag,setflag)    if (flags & checkflag) ret |= setflag
   CHECKSET (IFF_UP,          NETCODE_IFF_UP);
   CHECKSET (IFF_BROADCAST,   NETCODE_IFF_BROADCAST);
   CHECKSET (IFF_DEBUG,       NETCODE_IFF_DEBUG);
   CHECKSET (IFF_LOOPBACK,    NETCODE_IFF_LOOPBACK);
   CHECKSET (IFF_POINTOPOINT, NETCODE_IFF_POINTOPOINT);
   CHECKSET (IFF_RUNNING,     NETCODE_IFF_RUNNING);
   CHECKSET (IFF_NOARP,       NETCODE_IFF_NOARP);
   CHECKSET (IFF_PROMISC,     NETCODE_IFF_PROMISC);
   CHECKSET (IFF_NOTRAILERS,  NETCODE_IFF_NOTRAILERS);
   CHECKSET (IFF_ALLMULTI,    NETCODE_IFF_ALLMULTI);
   CHECKSET (IFF_MASTER,      NETCODE_IFF_MASTER);
   CHECKSET (IFF_SLAVE,       NETCODE_IFF_SLAVE);
   CHECKSET (IFF_MULTICAST,   NETCODE_IFF_MULTICAST);
   CHECKSET (IFF_PORTSEL,     NETCODE_IFF_PORTSEL);
   CHECKSET (IFF_AUTOMEDIA,   NETCODE_IFF_AUTOMEDIA);
   CHECKSET (IFF_DYNAMIC,     NETCODE_IFF_DYNAMIC);
   CHECKSET (IFF_LOWER_UP,    NETCODE_IFF_LOWER_UP);
   CHECKSET (IFF_DORMANT,     NETCODE_IFF_DORMANT);
   CHECKSET (IFF_ECHO,        NETCODE_IFF_ECHO);
#undef CHECKSET

   return ret;
}

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

      uint64_t lflags = if_reflag (if_tmp->ifa_flags);
      const char *lname = if_tmp->ifa_name;

      laddr    = netcode_util_sockaddr_to_str (if_tmp->ifa_addr);
      lnetmask = netcode_util_sockaddr_to_str (if_tmp->ifa_netmask);

      if (if_tmp->ifa_flags & IFF_BROADCAST) {
         lbroadcast = netcode_util_sockaddr_to_str (if_tmp->ifa_ifu.ifu_broadaddr);
      } else {
         lbroadcast = lstrdup ("");
      }

      if (if_tmp->ifa_flags & IFF_POINTOPOINT) {
         lp2p = netcode_util_sockaddr_to_str (if_tmp->ifa_ifu.ifu_dstaddr);
      } else {
         lp2p = lstrdup ("");
      }

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

#endif



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
   return error;
}


