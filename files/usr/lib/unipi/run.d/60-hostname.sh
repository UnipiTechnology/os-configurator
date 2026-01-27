#!/bin/sh

# create hostname based on PLC version and serial
#
# On RaspiOS ther is running NetworkManager
# Using unipihostname.service doesn't work because NM reverts hostname
# to permanent setting from /etc/hostname after dhcp lease
#
# We set permanent hostname here during os-configuration
ULIB_PATH=/usr/lib/unipi
newhostname=$($ULIB_PATH/unipihostname)
hostname=$(head -1 /etc/hostname)

if [ -z "$newhostname" ] || [ "$hostname" = "$newhostname" ]; then
    exit
fi


if [ "$hostname" = "unipi" ] || \
   [ "$hostname" = "raspberrypi" ] || \
   [ "$hostname" = "raspberrypi5" ] || \
   { echo "$hostname" | grep -Exq '^.*-sn[0-9]+$';}; then

    echo "Changing hostname from $hostname -> $newhostname"
    echo "$newhostname" > /etc/hostname
    if ! grep -q "^127.0.1.1  $newhostname$" /etc/hosts; then
        echo "127.0.1.1  $newhostname" >> /etc/hosts
    fi
    hostname "$newhostname"
fi
