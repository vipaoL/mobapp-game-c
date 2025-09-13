#!/bin/bash
./spd_dump --wait 300 fdl nor_fdl1.bin 0x40004000 fdl ../build/app.bin ram
./libc_server -- --bright 50

