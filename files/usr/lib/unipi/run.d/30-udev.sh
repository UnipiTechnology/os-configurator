#!/bin/sh


#UDEV="id0074_slot12"

SRC=/usr/share/unipi-os-configurator/udev
DST=/etc/udev/rules.d

if [ -n "$UDEV" ]; then 
    find ${DST}/ -type l -name 50-\* -exec rm -f \{\} \;
    for f in ${UDEV}; do
        ln -s "${SRC}/${f}.rules" "${DST}/50-${f}.rules"
    done
fi
