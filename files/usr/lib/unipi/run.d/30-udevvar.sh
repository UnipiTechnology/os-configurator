#!/bin/sh

# create special udev rule with counted nodes(subsystems)
# search used rules for tag #::: and replace <key> by incremented value

#UDEV="id0074_slot12"

SRC=/usr/share/unipi-os-configurator/udev
DST=/etc/udev/rules.d/45-udevvar.rules

if [ -n "$UDEV" ]; then 
(
cd "${SRC}" || exit 1
# append to words in udev list suffix .rules
udev_rules_1=$(echo "${UDEV}" | sed 's/\>/.rules/g')
# shellcheck disable=SC2086
awk '
/^#:::/ {
		gsub("^#:::\\s*", "")
		match($0, "<.*>")
		var = substr($0, RSTART, RLENGTH)
		val = variables[var]+1
		variables[var] = val
		gsub(var, val)
		print
}
' ${udev_rules_1} > "${DST}"
)
fi
