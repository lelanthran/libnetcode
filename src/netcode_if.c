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
   if (!src)
      return NULL;

   char *ret = malloc (strlen (src) + 1);
   if (ret)
      strcpy (ret, src);
   return ret;
}

/* ***************************************************************** */
struct netcode_if_t {
   uint64_t   if_flags;
   char      *if_name;
   char      *if_descr;
   char      *if_addr;
   char      *if_netmask;
   char      *if_broadcast;
   char      *if_p2paddr;
};

static void netcode_if_del (netcode_if_t *iface)
{
   if (!iface)
      return;

   free (iface->if_name);
   free (iface->if_descr);
   free (iface->if_addr);
   free (iface->if_netmask);
   free (iface->if_broadcast);
   free (iface->if_p2paddr);

   free (iface);
}

static netcode_if_t *netcode_if_new (uint64_t if_flags,
                                     const char *if_name,
                                     const char *if_descr,
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
   ret->if_descr     = lstrdup (if_descr);
   ret->if_addr      = lstrdup (if_addr);
   ret->if_netmask   = lstrdup (if_netmask);
   ret->if_broadcast = lstrdup (if_broadcast);
   ret->if_p2paddr   = lstrdup (if_p2paddr);

   if (!ret->if_name      ||
       !ret->if_descr     ||
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
#include <iptypes.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#if 0
https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
PIP_ADAPTER_ADDRESSES pAddresses = NULL;
ULONG outbuflen = 0;

IPHLPAPI_DLL_LINKAGE ULONG GetAdaptersAddresses(
  ULONG                 Family,              // AF_UNSPEC
  ULONG                 Flags,               // 0
  PVOID                 Reserved,            // NULL
  PIP_ADAPTER_ADDRESSES AdapterAddresses,    // &pAddresses
  PULONG                SizePointer          // &outbuflen
);



https://docs.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_addresses_lh
typedef struct _IP_ADAPTER_ADDRESSES_LH {
  union {
    ULONGLONG Alignment;
    struct {
      ULONG    Length;
      IF_INDEX IfIndex;
    };
  };
  struct _IP_ADAPTER_ADDRESSES_LH    *Next;
  PCHAR                              AdapterName;
  PIP_ADAPTER_UNICAST_ADDRESS_LH     FirstUnicastAddress;
  PIP_ADAPTER_ANYCAST_ADDRESS_XP     FirstAnycastAddress;
  PIP_ADAPTER_MULTICAST_ADDRESS_XP   FirstMulticastAddress;
  PIP_ADAPTER_DNS_SERVER_ADDRESS_XP  FirstDnsServerAddress;
  PWCHAR                             DnsSuffix;
  PWCHAR                             Description;
  PWCHAR                             FriendlyName;
  BYTE                               PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
  ULONG                              PhysicalAddressLength;
  union {
    ULONG Flags;
    struct {
      ULONG DdnsEnabled : 1;
      ULONG RegisterAdapterSuffix : 1;
      ULONG Dhcpv4Enabled : 1;
      ULONG ReceiveOnly : 1;
      ULONG NoMulticast : 1;
      ULONG Ipv6OtherStatefulConfig : 1;
      ULONG NetbiosOverTcpipEnabled : 1;
      ULONG Ipv4Enabled : 1;
      ULONG Ipv6Enabled : 1;
      ULONG Ipv6ManagedAddressConfigurationSupported : 1;
    };
  };
  ULONG                              Mtu;
  IFTYPE                             IfType;
  IF_OPER_STATUS                     OperStatus;
  IF_INDEX                           Ipv6IfIndex;
  ULONG                              ZoneIndices[16];
  PIP_ADAPTER_PREFIX_XP              FirstPrefix;
  ULONG64                            TransmitLinkSpeed;
  ULONG64                            ReceiveLinkSpeed;
  PIP_ADAPTER_WINS_SERVER_ADDRESS_LH FirstWinsServerAddress;
  PIP_ADAPTER_GATEWAY_ADDRESS_LH     FirstGatewayAddress;
  ULONG                              Ipv4Metric;
  ULONG                              Ipv6Metric;
  IF_LUID                            Luid;
  SOCKET_ADDRESS                     Dhcpv4Server;
  NET_IF_COMPARTMENT_ID              CompartmentId;
  NET_IF_NETWORK_GUID                NetworkGuid;
  NET_IF_CONNECTION_TYPE             ConnectionType;
  TUNNEL_TYPE                        TunnelType;
  SOCKET_ADDRESS                     Dhcpv6Server;
  BYTE                               Dhcpv6ClientDuid[MAX_DHCPV6_DUID_LENGTH];
  ULONG                              Dhcpv6ClientDuidLength;
  ULONG                              Dhcpv6Iaid;
  PIP_ADAPTER_DNS_SUFFIX             FirstDnsSuffix;
} IP_ADAPTER_ADDRESSES_LH, *PIP_ADAPTER_ADDRESSES_LH;


typedef struct _IP_ADAPTER_UNICAST_ADDRESS_LH {
  union {
    ULONGLONG Alignment;
    struct {
      ULONG Length;
      DWORD Flags;
    };
  };
  struct _IP_ADAPTER_UNICAST_ADDRESS_LH *Next;
  SOCKET_ADDRESS                        Address;
  IP_PREFIX_ORIGIN                      PrefixOrigin;
  IP_SUFFIX_ORIGIN                      SuffixOrigin;
  IP_DAD_STATE                          DadState;
  ULONG                                 ValidLifetime;
  ULONG                                 PreferredLifetime;
  ULONG                                 LeaseLifetime;
  UINT8                                 OnLinkPrefixLength;
} IP_ADAPTER_UNICAST_ADDRESS_LH, *PIP_ADAPTER_UNICAST_ADDRESS_LH;

#endif

static char *nmprefix_to_string (ULONG prefix, USHORT addr_family)
{
   struct sockaddr *sa = NULL;

   struct sockaddr_in sa4;
   struct sockaddr_in6 sa6;

   // Warning, this won't work on Windows/ARM
   if (addr_family == AF_INET) {
      uint32_t nm4 = 0;
      for (size_t i=0; i<prefix; i++) {
         nm4 |= (1 << i);
      }

      memset (&sa4, 0, sizeof sa4);
      sa = (struct sockaddr *)&sa4;
      sa4.sin_family = AF_INET;
      memcpy (&sa4.sin_addr, &nm4, sizeof sa4.sin_addr);
   }

   // Warning, this won't work on Windows/ARM
   if (addr_family == AF_INET6) {
      uint8_t nm6[16];
      memset (nm6, 0, sizeof nm6);
      for (size_t i=0; i<prefix; i++) {
         size_t bytenum = i / 8;
         size_t bitnum = i % 8;
         nm6[bytenum] |= (1 << bitnum);
      }

      memset (&sa6, 0, sizeof sa6);
      sa = (struct sockaddr *)&sa6;
      sa6.sin6_family = AF_INET6;
      memcpy (&sa6.sin6_addr, &nm6, sizeof sa6.sin6_addr);
   }

   if (!sa) {
      return lstrdup ("Unknown Address Family");
   }

   return netcode_util_sockaddr_to_str (sa);
}

netcode_if_t **netcode_if_list_new (void)
{
   bool error = true;
   netcode_if_t **ret = NULL;
   size_t nitems = 0;
   uint64_t if_flags = 0;
   char *if_name = NULL,
        *if_descr = NULL,
        *if_addr = NULL,
        *if_netmask = NULL,
        *if_broadcast = NULL,
        *if_p2paddr = NULL;

   ULONG outbuflen = 15 * 1024;
   PIP_ADAPTER_ADDRESSES addresses = malloc (outbuflen),
                         tmp = NULL;

   size_t attempts = 0;
   ULONG rc = ERROR_BUFFER_OVERFLOW;
   while ((rc = GetAdaptersAddresses (AF_UNSPEC,
                                      0,
                                      NULL,
                                      addresses,
                                      &outbuflen)) == ERROR_BUFFER_OVERFLOW) {
      if (attempts++ > 5) {
         NETCODE_UTIL_LOG ("Failed after five attempts to allocate memory [%lu]\n", outbuflen);
         break;
      }
      outbuflen *= 2;
      PIP_ADAPTER_ADDRESSES tmp = realloc (addresses, outbuflen);
      if (!tmp) {
         NETCODE_UTIL_LOG ("Out of memory realloc (%lu)\n", outbuflen);
         break;
      }
      addresses = tmp;
   }

   if (rc != NO_ERROR)  {
      NETCODE_UTIL_LOG ("Failed rc = [%lu]\n", rc);
      goto errorexit;
   }

   tmp = addresses;
   while (tmp) {
      PIP_ADAPTER_UNICAST_ADDRESS ip = tmp->FirstUnicastAddress;
      while (ip) {
         nitems++;
         ip = ip->Next;
      }
      tmp = tmp->Next;
   }

   if (!(ret = calloc (nitems + 1, sizeof *ret))) {
      NETCODE_UTIL_LOG ("Out of memory\n");
      goto errorexit;
   }

   tmp = addresses;
   size_t idx = 0;
   while (tmp) {

      PIP_ADAPTER_UNICAST_ADDRESS ip = tmp->FirstUnicastAddress;

      while (ip) {

         USHORT af = ip->Address.lpSockaddr->sa_family;

         size_t fname_len = wcstombs (NULL, tmp->FriendlyName, 0) + 1;
         size_t descr_len = wcstombs (NULL, tmp->Description, 0) + 1;
         size_t aname_len = strlen (tmp->AdapterName) + 1;

         free (if_name);      if_name = NULL;
         free (if_descr);     if_descr = NULL;
         free (if_addr);      if_addr = NULL;
         free (if_netmask);   if_netmask = NULL;
         free (if_broadcast); if_broadcast = NULL;
         free (if_p2paddr);   if_p2paddr = NULL;

         if_name = malloc (aname_len + 1);
         if (!if_name) {
             // TODO: Handle error
             goto errorexit;
         }
        strcpy (if_name, tmp->AdapterName);

         if_descr = malloc (fname_len + descr_len + 10);
         if (!if_descr) {
             // TODO: Handle error
             goto errorexit;
         }
         wcstombs (if_descr, tmp->FriendlyName, fname_len);
         strcat (if_descr, " (");
         wcstombs (&if_descr[fname_len + 1], tmp->Description, descr_len);
         strcat (if_descr, ")");

         if_flags = 0;
         if_addr = netcode_util_sockaddr_to_str (ip->Address.lpSockaddr);
         if_netmask = nmprefix_to_string (ip->OnLinkPrefixLength, af);
         if_broadcast = lstrdup ("");
         if_p2paddr = lstrdup ("");

         ret[idx] = netcode_if_new (if_flags, if_name,
                                              if_descr,
                                              if_addr,
                                              if_netmask,
                                              if_broadcast,
                                              if_p2paddr);
         if (!(ret[idx])) {
            NETCODE_UTIL_LOG ("OOM creating interface object\n");
            goto errorexit;
         }
         idx++;
         ip = ip->Next;
      }
      tmp = tmp->Next;
   }

   error = false;

errorexit:

   free (if_name);      if_name = NULL;
   free (if_descr);     if_descr = NULL;
   free (if_addr);      if_addr = NULL;
   free (if_netmask);   if_netmask = NULL;
   free (if_broadcast); if_broadcast = NULL;
   free (if_p2paddr);   if_p2paddr = NULL;

   free (addresses);
   if (error) {
     netcode_if_list_del (ret);
     ret = NULL;
   }

   return ret;
}

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

bool netcode_if_extract (const netcode_if_t *iface,
                         uint64_t   *dst_if_flags,
                         char      **dst_if_name,
                         char      **dst_if_descr,
                         char      **dst_if_addr,
                         char      **dst_if_netmask,
                         char      **dst_if_broadcast,
                         char      **dst_if_p2paddr)
{
   bool error = true;

   if (!iface)
      return false;

   if (dst_if_flags)
      *dst_if_flags = iface->if_flags;

#define CONDITIONAL_STRCPY(dst,src)      do {\
   if (dst) {\
      if (((*dst) = lstrdup (src))==NULL) {\
         NETCODE_UTIL_LOG ("Failed to copy [%s]\n", src);\
         goto errorexit;\
      }\
   }\
} while (0)

   CONDITIONAL_STRCPY (dst_if_name,       iface->if_name);
   CONDITIONAL_STRCPY (dst_if_descr,      iface->if_descr);
   CONDITIONAL_STRCPY (dst_if_addr,       iface->if_addr);
   CONDITIONAL_STRCPY (dst_if_netmask,    iface->if_netmask);
   CONDITIONAL_STRCPY (dst_if_broadcast,  iface->if_broadcast);
   CONDITIONAL_STRCPY (dst_if_p2paddr,    iface->if_p2paddr);

#undef CONDITIONAL_STRCPY

   error = false;
errorexit:
   return !error;
}


