#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "netcode_util.h"
#include "netcode_if.h"

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
// ...................
#endif


