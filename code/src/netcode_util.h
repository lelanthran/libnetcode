#ifndef H_NETCODE_UTIL
#define H_NETCODE_UTIL

#define NETCODE_UTIL_LOG(...)       do {\
   printf ("[%s:%i] ", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)

// This is only used by the test programs and can be ignored by the programmer.
#define NETCODE_TEST_SERVER         ("localhost")
#define NETCODE_TEST_TCP_PORT       (55155)
#define NETCODE_TEST_TCP_REQUEST    ("TCP request data")
#define NETCODE_TEST_TCP_RESPONSE   ("TCP response data")
#define NETCODE_TEST_UDP_PORT       (55156)
#define NETCODE_TEST_UDP_REQUEST    ("UDP request data")
#define NETCODE_TEST_UDP_RESPONSE   ("UDP response data")

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
};
#endif

#endif
