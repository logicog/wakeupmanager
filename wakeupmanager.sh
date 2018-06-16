#!/bin/bash
# Script for /lib/systemd/system-sleep
# to be run after suspend

case $1 in
  post)
    /usr/bin/kcminit kcm_wakeup
    ;;
esac
