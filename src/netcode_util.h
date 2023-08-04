#ifndef H_NETCODE_UTIL
#define H_NETCODE_UTIL

#include <stdbool.h>

#ifdef PLATFORM_Windows
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>
#endif

#ifdef PLATFORM_POSIX
#include <sys/socket.h>
#endif


#define NETCODE_UTIL_LOG(...)       do {\
   printf ("[%s:%i] ", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)

// This is only used by the test programs and can be ignored by the programmer.
#define NETCODE_TEST_SERVER            ("localhost")
#define NETCODE_TEST_TCP_PORT          (55155)
#define NETCODE_TEST_TCP_REQUEST       ("TCP request data")
#define NETCODE_TEST_TCP_RESPONSE      ("TCP response data")
#define NETCODE_TEST_UDP_CLIENT_PORT   (55156)
#define NETCODE_TEST_UDP_SERVER_PORT   (55157)
#define NETCODE_TEST_UDP_REQUEST1      ("UDP request data 1")
#define NETCODE_TEST_UDP_RESPONSE1     ("UDP response data 1")
#define NETCODE_TEST_UDP_REQUEST2      ("UDP request data 2")
#define NETCODE_TEST_UDP_RESPONSE2     ("UDP response data 2")

#ifdef PLATFORM_Windows
   typedef SOCKET socket_t;
#else
   typedef int socket_t;
#endif


#ifdef PLATFORM_Windows
#  define NETCODE_SOCK_VALID(x) (x!=INVALID_SOCKET)
#else
#  define NETCODE_INVALID_SOCKET  -1
#  define NETCODE_SOCK_VALID(x) (x >= 0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

   bool netcode_util_init (void);
   int netcode_util_clear_errno (void);
   int netcode_util_errno (void);
   const char *netcode_util_strerror (int err);

   int netcode_util_close (socket_t fd);

   // Caller must free the returned value
   char *netcode_util_sockaddr_to_str (const struct sockaddr *sa);


#ifdef __cplusplus
};
#endif

#endif
