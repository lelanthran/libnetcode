#!/bin/bash

valgrind ../debug/default-platform/bin/netcode_server_test.elf tcp_test &\
   sleep 1 ;\
   valgrind ../debug/default-platform/bin/netcode_client_test.elf tcp_test
