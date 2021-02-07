#!/bin/bash

if [ `uname -a | grep -ci linux` -gt 0 ]; then
   export PREFIX=wine
fi

$PREFIX ../debug/bin/x86_64-w64-mingw32/netcode_server_test.exe udp_test & sleep 1 ;\
$PREFIX ../debug/bin/x86_64-w64-mingw32/netcode_client_test.exe udp_test
