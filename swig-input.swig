%module netcode
%include "src/netcode_if.h"
%include "src/netcode_tcp.h"
%include "src/netcode_udp.h"
%include "src/netcode_util.h"

%{
#include "src/netcode_if.h"
#include "src/netcode_tcp.h"
#include "src/netcode_udp.h"
#include "src/netcode_util.h"
%}
