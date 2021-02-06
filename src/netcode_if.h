#ifndef H_NETCODE_IF
#define H_NETCODE_IF

typedef struct netcode_if_t netcode_if_t;

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
                             char      **dst_if_p2paddr);

#ifdef __cplusplus
};
#endif

#endif
