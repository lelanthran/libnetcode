#!/bin/bash

../debug/bin//x86_64-w64-mingw32/netcode_server_test.exe utcp_test & sleep 1 ; ../debug/bin/x86_64-w64-mingw32/netcode_client_test.exe utcp_test
