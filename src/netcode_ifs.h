#ifndef H_NETCODE_IFS
#define H_NETCODE_IFS

typedef struct netcode_ifs_t netcode_ifs_t;

#ifdef __cplusplus
extern "C" {
#endif

   netcode_ifs_t **netcode_ifs_list_new (void);
   void netcode_ifs_list_del (netcode_ifs_t **list);

   bool netcode_ifs_extract (const netcode_ifs_t *ifs,
                             uint64_t  *dst_if_flags,
                             char **dst_if_name,
                             char **dst_if_addr,
                             char **dst_if_netmask,
                             char **dst_if_broadcast,
                             char **dst_if_p2paddr);

#ifdef __cplusplus
};
#endif

#endif
