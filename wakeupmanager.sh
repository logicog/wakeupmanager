#!/bin/bash
# Script for /lib/systemd/system-sleep
# to be run before and after suspend

#case $1 in
#  post)
    logger "/lib/systemd/system-sleep/wakeupmanager.sh called"
    /usr/bin/configurewakeup
# /usr/bin/kcminit kcm_wakeup
    logger "/lib/systemd/system-sleep/wakeupmanager.sh done"
#    ;;
#esac
