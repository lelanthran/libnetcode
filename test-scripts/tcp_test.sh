#!/bin/bash

export DIRNAME="`dirname $0`"
pushd $DIRNAME

valgrind ../recent/bin/netcode_server_test.elf tcp_test & sleep 1 ; valgrind ../recent/bin/netcode_client_test.elf tcp_test
