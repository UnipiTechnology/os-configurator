#inclode <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

//extern FILE *stderr;

//ULIB_PATH="/usr/lib/unipi"

int create_alternate_script(int altpart, char* bootdev)
{
    bootdevtmp = strdup(bootdev);
    char* mmc = basename(bootdevtmp);
    if (strncmp(mmc, "mmcblk", strlen("mmcblk") == 0)  {
        strcpy(mmc+strlen("mmc"), mmc+strlen("mmcblk"));
    }
    int f = open("/boot/tryboot", O_WRONLY+O_CREAT+O_TRUNC);
    if (f < 0) {
        fprintf(stderr, "Error %d: %s", errno, strerror(errno));
        goto final;
    }
    dprintf(f, "%s:%d", mmc, altpart);
    close(f);
final:
    free(bootdevtmp);
}


set_boot_permanent()
{
    dev=$1
    old=$2
    new=$3
    [ "$old" = "$new" ] && return
    tmppart=$(mktemp) || return 1
    sfdisk -d "$dev" > "$tmppart"
    if grep -q 'label: gpt' "$tmppart"; then
        sfdisk --no-reread --no-tell-kernel --quiet --part-attrs "$dev" "$new" LegacyBIOSBootable
        sfdisk --no-reread --no-tell-kernel --quiet --part-attrs "$dev" "$old" ""
        #sed "s|,[[:blank:]]*attrs=.*$||;\#$dev$new#s|$|, attrs=\"LegacyBIOSBootable\"|"
    else
        #echo "Dos type partition"
        sfdisk --no-reread --no-tell-kernel --quiet --activate "$dev" "$new"
    fi
    rm -f "$tmppart"
}

swap_part_number()
{
    if ! [ -r /etc/default/switchboot ]; then
        echo "Cannot read configuration of A/B system from /etc/default/switchboot" >&2
        exit 1
    fi
    unset A
    unset B
    . /etc/default/switchboot
    if [ -z "$A" ] || [ -z "$B" ]; then
        echo "Incorrect definition of variables A and B in /etc/default/switchboot" >&2
        echo "Define in the file for example A=2 and B=3" >&2
        exit 1
    fi
    if [ "$1" = "$B" ]; then
        echo "$A"
    elif [ "$1" = "$A" ]; then
        echo "$B"
    else
        echo "Bootpart unknown ($1)! Alternate boot not set!" >&2
        exit 1
    fi
}
