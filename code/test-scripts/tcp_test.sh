#!/bin/bash

valgrind ../debug/bin/x86_64-linux-gnu/netcode_server_test.elf tcp_test & sleep 1 ; valgrind ../debug/bin/x86_64-linux-gnu/netcode_client_test.elf tcp_test
