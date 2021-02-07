#!/bin/bash

if [ `uname -a | grep -ci linux` -gt 0 ]; then
   export PREFIX=wine
fi

rm win_server_result.txt
rm win_client_result.txt

$PREFIX ../debug/bin/x86_64-w64-mingw32/netcode_server_test.exe udp_test &> win_server_result.txt &
sleep 1
$PREFIX ../debug/bin/x86_64-w64-mingw32/netcode_client_test.exe udp_test &> win_client_result.txt

cat win_server_result.txt
cat win_client_result.txt

