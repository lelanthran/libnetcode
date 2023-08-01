#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "netcode_util.h"

/* ***************************************************************** */
#if defined (OSTYPE_Darwin)
#define SOCK_CLOEXEC       (0)
#define accept4(x,y,z,A)   accept (x,y,(int *)z)
#define SEND(x,y,z)        send (x,y,z, 0)
#endif

/* ***************************************************************** */
#ifdef PLATFORM_Windows
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>
#include <ws2tcpip.h>

//    Using FormatMessage to produce error strings from numbers is
// insane.  The person who does that sort of thing needs to have an
// eye kept on them.
//
//    Preferably from a safe distance.
//

static const struct {
   int errcode;
   const char *errmsg;
} msgs[] = {
#define MSG(x,y)     {  x, #x ": " y }
MSG (WSA_INVALID_HANDLE,         "Specified event object handle is invalid.\n    An application attempts to use an event object, but the specified handle is not valid.\n"    ),
MSG (WSA_NOT_ENOUGH_MEMORY,      "Insufficient memory available.\n    An application used a Windows Sockets function that directly maps to a Windows function. The Windows function is indicating a lack of required memory resources.\n"    ),
MSG (WSA_INVALID_PARAMETER,      "One or more parameters are invalid.\n    An application used a Windows Sockets function which directly maps to a Windows function. The Windows function is indicating a problem with one or more parameters.\n"    ),
MSG (WSA_OPERATION_ABORTED,      "Overlapped operation aborted.\n    An overlapped operation was canceled due to the closure of the socket, or the execution of the SIO_FLUSH command in WSAIoctl.\n"    ),
MSG (WSA_IO_INCOMPLETE,          "Overlapped I/O event object not in signaled state.\n    The application has tried to determine the status of an overlapped operation which is not yet completed. Applications that use WSAGetOverlappedResult (with the fWait flag set to FALSE) in a polling mode to determine when an overlapped operation has completed, get this error code until the operation is complete.\n"    ),
MSG (WSA_IO_PENDING,             "Overlapped operations will complete later.\n    The application has initiated an overlapped operation that cannot be completed immediately. A completion indication will be given later when the operation has been completed.\n"    ),
MSG (WSAEINTR,                   "Interrupted function call.\n    A blocking operation was interrupted by a call to WSACancelBlockingCall.\n"    ),
MSG (WSAEBADF,                   "File handle is not valid.\n    The file handle supplied is not valid.\n"    ),
MSG (WSAEACCES,                  "Permission denied.\n    An attempt was made to access a socket in a way forbidden by its access permissions. An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST).\n Another possible reason for the WSAEACCES error is that when the bind function is called (on Windows NT 4.0 with SP4 and later), another application, service, or kernel mode driver is bound to the same address with exclusive access. Such exclusive access is a new feature of Windows NT 4.0 with SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option.\n"    ),
MSG (WSAEFAULT,                  "Bad address.\n    The system detected an invalid pointer address in attempting to use a pointer argument of a call. This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small. For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr).\n"    ),
MSG (WSAEINVAL,                  "Invalid argument.\n    Some invalid argument was supplied (for example, specifying an invalid level to the setsockopt function). In some instances, it also refers to the current state of the socket—for instance, calling accept on a socket that is not listening.\n"    ),
MSG (WSAEMFILE,                  "Too many open files.\n    Too many open sockets. Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread.\n"    ),
MSG (WSAEWOULDBLOCK,             "Resource temporarily unavailable.\n    This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket. It is a nonfatal error, and the operation should be retried later. It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established.\n"    ),
MSG (WSAEINPROGRESS,             "Operation now in progress.\n    A blocking operation is currently executing. Windows Sockets only allows a single blocking operation—per- task or thread—to be outstanding, and if any other function call is made (whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error.\n"    ),
MSG (WSAEALREADY,                "Operation already in progress.\n    An operation was attempted on a nonblocking socket with an operation already in progress—that is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request (WSAAsyncGetXbyY) that has already been canceled or completed.\n"    ),
MSG (WSAENOTSOCK,                "Socket operation on nonsocket.\n    An operation was attempted on something that is not a socket. Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid.\n"    ),
MSG (WSAEDESTADDRREQ,            "Destination address required.\n    A required address was omitted from an operation on a socket. For example, this error is returned if sendto is called with the remote address of ADDR_ANY.\n"    ),
MSG (WSAEMSGSIZE,                "Message too long.\n    A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself.\n"    ),
MSG (WSAEPROTOTYPE,              "Protocol wrong type for socket.\n    A protocol was specified in the socket function call that does not support the semantics of the socket type requested. For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM.\n"    ),
MSG (WSAENOPROTOOPT,             "Bad protocol option.\n    An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call.\n"    ),
MSG (WSAEPROTONOSUPPORT,         "Protocol not supported.\n    The requested protocol has not been configured into the system, or no implementation for it exists. For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol.\n"    ),
MSG (WSAESOCKTNOSUPPORT,         "Socket type not supported.\n    The support for the specified socket type does not exist in this address family. For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all.\n"    ),
MSG (WSAEOPNOTSUPP,              "Operation not supported.\n    The attempted operation is not supported for the type of object referenced. Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket.\n"    ),
MSG (WSAEPFNOSUPPORT,            "Protocol family not supported.\n    The protocol family has not been configured into the system or no implementation for it exists. This message has a slightly different meaning from WSAEAFNOSUPPORT. However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT.\n"    ),
MSG (WSAEAFNOSUPPORT,            "Address family not supported by protocol family.\n    An address incompatible with the requested protocol was used. All sockets are created with an associated address family (that is, AF_INET for Internet Protocols) and a generic protocol type (that is, SOCK_STREAM). This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto.\n"    ),
MSG (WSAEADDRINUSE,              "Address already in use.\n    Typically, only one usage of each socket address (protocol/IP address/port) is permitted. This error occurs if an application attempts to bind a socket to an IP address/port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing. For server applications that need to bind multiple sockets to the same port number, consider using setsockopt (SO_REUSEADDR). Client applications usually need not call bind at all—connect chooses an unused port automatically. When bind is called with a wildcard address (involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed. This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf.\n"    ),
MSG (WSAEADDRNOTAVAIL,           "Cannot assign requested address.\n    The requested address is not valid in its context. This normally results from an attempt to bind to an address that is not valid for the local computer. This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer (for example, address or port 0).\n"    ),
MSG (WSAENETDOWN,                "Network is down.\n    A socket operation encountered a dead network. This could indicate a serious failure of the network system (that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself.\n"    ),
MSG (WSAENETUNREACH,             "Network is unreachable.\n    A socket operation was attempted to an unreachable network. This usually means the local software knows no route to reach the remote host.\n"    ),
MSG (WSAENETRESET,               "Network dropped connection on reset.\n    The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed.\n"    ),
MSG (WSAECONNABORTED,            "Software caused connection abort.\n    An established connection was aborted by the software in your host computer, possibly due to a data transmission time-out or protocol error.\n"    ),
MSG (WSAECONNRESET,              "Connection reset by peer.\n    An existing connection was forcibly closed by the remote host. This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close (see setsockopt for more information on the SO_LINGER option on the remote socket). This error may also result if a connection was broken due to keep-alive activity detecting a failure while one or more operations are in progress. Operations that were in progress fail with WSAENETRESET. Subsequent operations fail with WSAECONNRESET.\n"    ),
MSG (WSAENOBUFS,                 "No buffer space available.\n    An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.\n"    ),
MSG (WSAEISCONN,                 "Socket is already connected.\n    A connect request was made on an already-connected socket. Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket (for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence.\n"    ),
MSG (WSAENOTCONN,                "Socket is not connected.\n    A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied. Any other type of operation might also return this error—for example, setsockopt setting SO_KEEPALIVE if the connection has been reset.\n"    ),
MSG (WSAESHUTDOWN,               "Cannot send after socket shutdown.\n    A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued.\n"    ),
MSG (WSAETOOMANYREFS,            "Too many references.\n    Too many references to some kernel object.\n"    ),
MSG (WSAETIMEDOUT,               "Connection timed out.\n    A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond.\n"    ),
MSG (WSAECONNREFUSED,            "Connection refused.\n    No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign host—that is, one with no server application running.\n"    ),
MSG (WSAELOOP,                   "Cannot translate name.\n    Cannot translate a name.\n"    ),
MSG (WSAENAMETOOLONG,            "Name too long.\n    A name component or a name was too long.\n"    ),
MSG (WSAEHOSTDOWN,               "Host is down.\n    A socket operation failed because the destination host is down. A socket operation encountered a dead host. Networking activity on the local host has not been initiated. These conditions are more likely to be indicated by the error WSAETIMEDOUT.\n"    ),
MSG (WSAEHOSTUNREACH,            "No route to host.\n    A socket operation was attempted to an unreachable host. See WSAENETUNREACH.\n"    ),
MSG (WSAENOTEMPTY,               "Directory not empty.\n    Cannot remove a directory that is not empty.\n"    ),
MSG (WSAEPROCLIM,                "Too many processes.\n    A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously. WSAStartup may fail with this error if the limit has been reached.\n"    ),
MSG (WSAEUSERS,                  "User quota exceeded.\n    Ran out of user quota.\n"    ),
MSG (WSAEDQUOT,                  "Disk quota exceeded.\n    Ran out of disk quota.\n"    ),
MSG (WSAESTALE,                  "Stale file handle reference.\n    The file handle reference is no longer available.\n"    ),
MSG (WSAEREMOTE,                 "Item is remote.\n    The item is not available locally.\n"    ),
MSG (WSASYSNOTREADY,             "Network subsystem is unavailable.\n    This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable. Users should check:\n That the appropriate Windows Sockets DLL file is in the current path.\n    That they are not trying to use more than one Windows Sockets implementation simultaneously. If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded.\n The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly.\n"    ),
MSG (WSAVERNOTSUPPORTED,         "Winsock.dll version out of range.\n    The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application. Check that no old Windows Sockets DLL files are being accessed.\n"    ),
MSG (WSANOTINITIALISED,          "Successful WSAStartup not yet performed.\n    Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times.\n"    ),
MSG (WSAEDISCON,                 "Graceful shutdown in progress.\n    Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence.\n"    ),
MSG (WSAENOMORE,                 "No more results.\n    No more results can be returned by the WSALookupServiceNext function.\n"    ),
MSG (WSAECANCELLED,              "Call has been canceled.\n    A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.\n"    ),
MSG (WSAEINVALIDPROCTABLE,       "Procedure call table is invalid.\n    The service provider procedure call table is invalid. A service provider returned a bogus procedure table to Ws2_32.dll. This is usually caused by one or more of the function pointers being NULL.\n"    ),
MSG (WSAEINVALIDPROVIDER,        "Service provider is invalid.\n    The requested service provider is invalid. This error is returned by the WSCGetProviderInfo and WSCGetProviderInfo32 functions if the protocol entry specified could not be found. This error is also returned if the service provider returned a version number other than 2.0.\n"    ),
MSG (WSAEPROVIDERFAILEDINIT,     "Service provider failed to initialize.\n    The requested service provider could not be loaded or initialized. This error is returned if either a service provider's DLL could not be loaded (LoadLibrary failed) or the provider's WSPStartup or NSPStartup function failed.\n"    ),
MSG (WSASYSCALLFAILURE,          "System call failure.\n    A system call that should never fail has failed. This is a generic error code, returned under various conditions.\n Returned when a system call that should never fail does fail. For example, if a call to WaitForMultipleEvents fails or one of the registry functions fails trying to manipulate the protocol/namespace catalogs.\n    Returned when a provider does not return SUCCESS and does not provide an extended error code. Can indicate a service provider implementation error.\n"    ),
MSG (WSASERVICE_NOT_FOUND,       "Service not found.\n    No such service is known. The service cannot be found in the specified name space.\n"    ),
MSG (WSATYPE_NOT_FOUND,          "Class type not found.\n    The specified class was not found.\n"    ),
MSG (WSA_E_NO_MORE,              "No more results.\n    No more results can be returned by the WSALookupServiceNext function.\n"    ),
MSG (WSA_E_CANCELLED,            "Call was canceled.\n    A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.\n"    ),
MSG (WSAEREFUSED,                "Database query was refused.\n    A database query failed because it was actively refused.\n"    ),
MSG (WSAHOST_NOT_FOUND,          "Host not found.\n    No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried. This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database.\n"    ),
MSG (WSATRY_AGAIN,               "Nonauthoritative host not found.\n    This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server. A retry at some time later may be successful.\n"    ),
MSG (WSANO_RECOVERY,             "This is a nonrecoverable error.\n    This indicates that some sort of nonrecoverable error occurred during a database lookup. This may be because the database files (for example, BSD-compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error.\n"    ),
MSG (WSANO_DATA,                 "Valid name, no data record of requested type.\n    The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for. The usual example for this is a host name-to-address translation attempt (using gethostbyname or WSAAsyncGetHostByName) which uses the DNS (Domain Name Server). An MX record is returned but no A record—indicating the host itself exists, but is not directly reachable.\n"    ),
MSG (WSA_QOS_RECEIVERS,          "QoS receivers.\n    At least one QoS reserve has arrived.\n"    ),
MSG (WSA_QOS_SENDERS,            "QoS senders.\n    At least one QoS send path has arrived.\n"    ),
MSG (WSA_QOS_NO_SENDERS,         "No QoS senders.\n    There are no QoS senders.\n"    ),
MSG (WSA_QOS_NO_RECEIVERS,       "QoS no receivers.\n    There are no QoS receivers.\n"    ),
MSG (WSA_QOS_REQUEST_CONFIRMED,  "QoS request confirmed.\n    The QoS reserve request has been confirmed.\n"    ),
MSG (WSA_QOS_ADMISSION_FAILURE,  "QoS admission error.\n    A QoS error occurred due to lack of resources.\n"    ),
MSG (WSA_QOS_POLICY_FAILURE,     "QoS policy failure.\n    The QoS request was rejected because the policy system couldn't allocate the requested resource within the existing policy.\n"    ),
MSG (WSA_QOS_BAD_STYLE,          "QoS bad style.\n    An unknown or conflicting QoS style was encountered.\n"    ),
MSG (WSA_QOS_BAD_OBJECT,         "QoS bad object.\n    A problem was encountered with some part of the filterspec or the provider-specific buffer in general.\n"    ),
MSG (WSA_QOS_TRAFFIC_CTRL_ERROR, "QoS traffic control error.\n    An error with the underlying traffic control (TC) API as the generic QoS request was converted for local enforcement by the TC API. This could be due to an out of memory error or to an internal QoS provider error.\n"    ),
MSG (WSA_QOS_GENERIC_ERROR,      "QoS generic error.\n    A general QoS error.\n"    ),
MSG (WSA_QOS_ESERVICETYPE,       "QoS service type error.\n    An invalid or unrecognized service type was found in the QoS flowspec.\n"    ),
MSG (WSA_QOS_EFLOWSPEC,          "QoS flowspec error.\n    An invalid or inconsistent flowspec was found in the QOS structure.\n"    ),
MSG (WSA_QOS_EPROVSPECBUF,       "Invalid QoS provider buffer.\n    An invalid QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_EFILTERSTYLE,       "Invalid QoS filter style.\n    An invalid QoS filter style was used.\n"    ),
MSG (WSA_QOS_EFILTERTYPE,        "Invalid QoS filter type.\n    An invalid QoS filter type was used.\n"    ),
MSG (WSA_QOS_EFILTERCOUNT,       "Incorrect QoS filter count.\n    An incorrect number of QoS FILTERSPECs were specified in the FLOWDESCRIPTOR.\n"    ),
MSG (WSA_QOS_EOBJLENGTH,         "Invalid QoS object length.\n    An object with an invalid ObjectLength field was specified in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_EFLOWCOUNT,         "Incorrect QoS flow count.\n    An incorrect number of flow descriptors was specified in the QoS structure.\n"    ),
MSG (WSA_QOS_EUNKOWNPSOBJ,       "Unrecognized QoS object.\n    An unrecognized object was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_EPOLICYOBJ,         "Invalid QoS policy object.\n    An invalid policy object was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_EFLOWDESC,          "Invalid QoS flow descriptor.\n    An invalid QoS flow descriptor was found in the flow descriptor list.\n"    ),
MSG (WSA_QOS_EPSFLOWSPEC,        "Invalid QoS provider-specific flowspec.\n    An invalid or inconsistent flowspec was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_EPSFILTERSPEC,      "Invalid QoS provider-specific filterspec.\n    An invalid FILTERSPEC was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_ESDMODEOBJ,         "Invalid QoS shape discard mode object.\n    An invalid shape discard mode object was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_ESHAPERATEOBJ,      "Invalid QoS shaping rate object.\n    An invalid shaping rate object was found in the QoS provider-specific buffer.\n"    ),
MSG (WSA_QOS_RESERVED_PETYPE,    "Reserved policy QoS element type.\n    A reserved policy element was found in the QoS provider-specific buffer.\n"    ),
};

size_t msgs_len = sizeof msgs / sizeof msgs[0];

static const char *wsa_errmsg (int errcode)
{
   static char unknown_message[50];

   for (size_t i=0; i<msgs_len; i++) {
      if (msgs[i].errcode == errcode)
         return msgs[i].errmsg;
   }

   snprintf (unknown_message, sizeof unknown_message, "WSA Code [%i] is unknown", errcode);
   return unknown_message;
}


#define SOCK_CLOEXEC       (0)
#define MSG_DONTWAIT       (0)
#define close(x)           closesocket (x)
#define accept4(x,y,z,A)   accept (x,y,(int *)z)
#define SEND(x,y,z)        send (x,y,z, 0)
#define read(x,y,z)        recv (x,(char *)y,z, 0)

const char *hstrerror (int error);

static bool initialised = false;

#define SAFETY_CHECK       do {\
   if (!initialised) {\
      wsa_netcode_init (); \
      initialised = true;\
   }\
} while (0)

static WSADATA xp_wsaData;
static int xp_wsaInitialised = 0;

static bool wsa_netcode_init (void)
{
   if (xp_wsaInitialised > 0)
      return true;

   atexit ((void (*)(void))WSACleanup);

   int result = WSAStartup (MAKEWORD(2,2), &xp_wsaData);
   if (result!=0) {
      NETCODE_UTIL_LOG ("Critical: winsock could not be initiliased - %i\n", result);
      return false;
   }

   xp_wsaInitialised = 1;
   return true;
}

#endif

/* ***************************************************************** */
#ifdef PLATFORM_POSIX
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>


#ifndef OSTYPE_Darwin
/* GCC in strict mode leaves out some libraries it really shouldn't.
 * Rather than spend the next twenty minutes figuring out which set
 * of flags, pragmas or -D... will make GCC use the arpa/inet header,
 * I'll just put this here.
 */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
const char *hstrerror (int err);

#define SEND(x,y,z)        send (x,y,z, MSG_NOSIGNAL)

#endif


static bool initialised = true;
#define SAFETY_CHECK    ;

#endif

#ifndef SAFETY_CHECK
#error SAFETY_CHECK not defined - platform variable undefined?
#endif


bool netcode_util_init (void)
{
   SAFETY_CHECK;
   return initialised;
}


int netcode_util_clear_errno (void)
{
   errno = 0;
#ifndef PLATFORM_Windows
   h_errno = 0;
#endif
   return 0;
}

int netcode_util_errno (void)
{
#ifdef PLATFORM_Windows
   return WSAGetLastError ();
#else
   return errno ? errno : h_errno;
#endif
}

const char *netcode_util_strerror (int err)
{
#ifdef PLATFORM_Windows
   return wsa_errmsg (err);
#else
   return errno ? strerror (err) : hstrerror (err);
#endif
}

int netcode_util_close (int fd)
{
   /* ****************************************
    * 1. Call close() on the socket descriptor
    */
   SAFETY_CHECK;
#ifdef PLATFORM_Windows
   shutdown (fd, SD_BOTH);
#else
   shutdown (fd, SHUT_RDWR);
#endif
   return close (fd);
}

#ifdef PLATFORM_Windows

char *netcode_util_sockaddr_to_str (const struct sockaddr *sa)
{
   SAFETY_CHECK;

   LPSTR ret = NULL;
   DWORD ret_len = 0;
   int rc = 0;

   struct sockaddr_in sa4;
   struct sockaddr_in6 sa6;
   struct sockaddr *local_sa = NULL;
   size_t local_sa_len = 0;

   if (sa->sa_family == AF_INET) {
      memset (&sa4, 0, sizeof sa4);
      sa4.sin_family = ((struct sockaddr_in *)sa)->sin_family;
      sa4.sin_addr = ((struct sockaddr_in *)sa)->sin_addr;
      local_sa = (struct sockaddr *)&sa4;
      local_sa_len = sizeof sa4;
   }
   if (sa->sa_family == AF_INET6) {
      memset (&sa6, 0, sizeof sa6);
      sa6.sin6_family = ((struct sockaddr_in6 *)sa)->sin6_family;
      sa6.sin6_addr = ((struct sockaddr_in6 *)sa)->sin6_addr;
      local_sa = (struct sockaddr *)&sa6;
      local_sa_len = sizeof sa6;
   }


   if (!(ret = malloc (1)))
      return NULL;
   ret_len = 1;

   rc = WSAAddressToStringA (local_sa,
                             local_sa_len,
                             NULL,
                             ret,
                             &ret_len);
   if (rc!=-1 || ret_len==0 || (WSAGetLastError ()!=WSAEFAULT))
      return NULL;

   free (ret);
   if (!(ret = malloc (ret_len + 1)))
      return NULL;

   rc = WSAAddressToStringA (local_sa,
                             local_sa_len,
                             NULL,
                             ret,
                             &ret_len);
   if (rc != 0) {
      free (ret);
      ret = NULL;
   }

   return ret;
}
#endif

#ifdef PLATFORM_POSIX

char *netcode_util_sockaddr_to_str (const struct sockaddr *sa)
{
#define UNKNOWN_AF      ("Unknown Address Family")
   char *ret = NULL;
   if (!sa) {
      ret = calloc (1, 2);
      ret[0] = 0;
      return ret;
   }

   switch (sa->sa_family) {
      case AF_INET:
         if (!(ret = calloc (1, INET_ADDRSTRLEN + 1)))
            return NULL;
         inet_ntop (AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    ret, INET_ADDRSTRLEN);
         break;

      case AF_INET6:
         if (!(ret = calloc (1, INET6_ADDRSTRLEN + 1)))
            return NULL;
         inet_ntop (AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    ret, INET6_ADDRSTRLEN);
         break;

      default:
         if (!(ret = calloc (1, strlen (UNKNOWN_AF) + 1)))
            return NULL;
         strcpy (ret, UNKNOWN_AF);
         break;
   }

   return ret;
}
#endif
