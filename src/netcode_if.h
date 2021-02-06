#ifndef H_NETCODE_IF
#define H_NETCODE_IF

#include <stdint.h>
#include <stdbool.h>

typedef struct netcode_if_t netcode_if_t;

#define NETCODE_IFF_UP              (1 << 0)
#define NETCODE_IFF_BROADCAST       (1 << 1)
#define NETCODE_IFF_DEBUG           (1 << 2)
#define NETCODE_IFF_LOOPBACK        (1 << 3)
#define NETCODE_IFF_POINTOPOINT     (1 << 4)
#define NETCODE_IFF_RUNNING         (1 << 5)
#define NETCODE_IFF_NOARP           (1 << 6)
#define NETCODE_IFF_PROMISC         (1 << 7)
#define NETCODE_IFF_NOTRAILERS      (1 << 8)
#define NETCODE_IFF_ALLMULTI        (1 << 9)
#define NETCODE_IFF_MASTER          (1 << 10)
#define NETCODE_IFF_SLAVE           (1 << 11)
#define NETCODE_IFF_MULTICAST       (1 << 12)
#define NETCODE_IFF_PORTSEL         (1 << 13)
#define NETCODE_IFF_AUTOMEDIA       (1 << 14)
#define NETCODE_IFF_DYNAMIC         (1 << 15)
#define NETCODE_IFF_LOWER_UP        (1 << 16)
#define NETCODE_IFF_DORMANT         (1 << 17)
#define NETCODE_IFF_ECHO            (1 << 18)


#ifdef __cplusplus
extern "C" {
#endif

   netcode_if_t **netcode_if_list_new (void);
   void netcode_if_list_del (netcode_if_t **list);

   bool netcode_if_extract (const netcode_if_t *interface,
                            uint64_t   *dst_if_flags,
                            char      **dst_if_name,
                            char      **dst_if_addr,
                            char      **dst_if_netmask,
                            char      **dst_if_broadcast,
                            char      **dst_if_p4paddr);

#ifdef __cplusplus
};
#endif

#endif
