#!/bin/sh

if ! [ "$VERBOSE" = "1" ]; then
    exec >/dev/null
fi

MAX_WAIT=10
if systemctl -q is-enabled unipi-resize-partition.service 2>/dev/null; then
    echo "Waiting for partition resize"
    while systemctl -q is-enabled unipi-resize-partition.service; do
        [ "${MAX_WAIT}" = "0" ] && break
        MAX_WAIT=$((MAX_WAIT-1))
        sleep 1
    done
fi

if systemctl -q is-enabled regenerate_ssh_host_keys.service 2>/dev/null; then
    echo "Waiting for partition resize"
    while systemctl -q is-enabled regenerate_ssh_host_keys.service; do
        [ "${MAX_WAIT}" = "0" ] && break
        MAX_WAIT=$((MAX_WAIT-1))
        sleep 1
    done
fi

exit 0
