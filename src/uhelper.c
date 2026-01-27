
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <regex.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

//#define RUNDIR "/home/bokula/tmp/run"
#define RUNDIR "/run/unipi-plc"
#define U_SLOT_DIR  RUNDIR "/by-slot/slot%s/%s"
#define U_SYS_DIR   RUNDIR "/by-sys/%s"

extern char **environ;

int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';
            if (mkdir(_path, 0755) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }
            *p = '/';
        }
    }

    if (mkdir(_path, 0755) != 0) {
        if (errno != EEXIST)
            return -1; 
    }

    return 0;
}

int write_interface(char *dir, char* interface)
{
	char tmp[PATH_MAX];
	int fd;
	snprintf(tmp, PATH_MAX-1, "%s/%s", dir, "interface");
	fd = open(tmp, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		return fd;
	}
	write(fd, interface, strlen(interface));
	close(fd);
	return 0;
}

int ln_sf(const char *target, const char* dir, const char *linkpath)
{
	char tmp[PATH_MAX];
	snprintf(tmp, PATH_MAX-1, "%s/%s", dir, linkpath);
	unlink(tmp);
	symlink(target, tmp);
	return 0;
}

int unlink_f(const char* dir, const char *linkpath)
{
	char tmp[PATH_MAX];
	snprintf(tmp, PATH_MAX-1, "%s/%s", dir, linkpath);
	unlink(tmp);
	return 0;
}

int create_udev_infofile(char *dir)
{
	char tmp[PATH_MAX];
	int fd;
	snprintf(tmp, PATH_MAX-1, "%s/%s", dir, "udev_info");
	fd = open(tmp, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		return fd;
	}

	for (char **env_item = environ; *env_item != 0; env_item++) {
		write(fd, *env_item, strlen(*env_item));
		write(fd, "\n", 1);
        }

	close(fd);
	return 0;
}

int unipi_id(char* u_func)
{
	char tmp[PATH_MAX];
	char *dev;

	mkdir_p(RUNDIR);
	dev = getenv("DEVPATH");
	if (dev) {
		snprintf(tmp, PATH_MAX-1, "/sys%s/unipi-id", dev);
		ln_sf(tmp, RUNDIR, u_func);
	}
	return 0;
}

char u_sys_dir[PATH_MAX];
char u_slot_dir[PATH_MAX];
char tmp[PATH_MAX];
char sysdev[PATH_MAX];

int main(int argc, char **argv)
{
	char *u_sys, *u_func, *u_slot;
	char *dev, *interface;
	char *action = getenv("ACTION");
	char *subsystem = getenv("SUBSYSTEM");
	char *uhelper_link;

	if ((action == NULL) || (argc < 3) ) {
		return 1;
	}
	if (strcmp(argv[1], "unipi-id")==0) {
		return unipi_id(argv[2]);
	}
	if (argc < 4) {
		return 1;
	}
	u_sys = argv[1];
	u_func = argv[2];
	u_slot = argv[3];

	snprintf(u_slot_dir, PATH_MAX-1, U_SLOT_DIR, u_slot, u_sys);
	snprintf(u_sys_dir, PATH_MAX-1, U_SYS_DIR, u_sys);

	if (strcmp(action, "add")==0) {
		mkdir_p(u_slot_dir);
		mkdir_p(u_sys_dir);
		if (subsystem && (strcmp(subsystem, "net")==0)) {
			interface = getenv("INTERFACE");
			if (interface) {
				write_interface(u_slot_dir, interface);
				write_interface(u_sys_dir, interface);
			}
		} else if (subsystem &&
				((strcmp(subsystem, "tty")==0)||\
				(strcmp(subsystem, "gpio")==0)||\
				(strcmp(subsystem, "block")==0)||\
				(strcmp(subsystem, "mtd")==0)||\
				(strcmp(subsystem, "tpm")==0)||\
				(strcmp(subsystem, "tpmrm")==0)||\
				(strcmp(subsystem, "spidev")==0))) {
			dev = getenv("DEVNAME");
			if (dev) {
				ln_sf(dev, u_slot_dir, u_func);
				ln_sf(dev, u_sys_dir,  u_func);
				//create_udev_infofile(u_slot_dir); //// TODO - delete file on remove
				//create_udev_infofile(u_sys_dir); // TODO - delete file on remove
				return 0;
			}
		}
		dev = getenv("DEVPATH");
		if (dev) {
			uhelper_link = getenv("UHELPER_LINK");
			if (uhelper_link)
				snprintf(sysdev, PATH_MAX-1, "/sys%s/%s", dev, uhelper_link);
			else
				snprintf(sysdev, PATH_MAX-1, "/sys%s", dev);
			ln_sf(sysdev, u_slot_dir, u_func);
			ln_sf(sysdev, u_sys_dir,  u_func);
		}

	} else if (strcmp(action, "remove")==0) {
		if (subsystem && (strcmp(subsystem, "net")==0)) {
			unlink_f(u_slot_dir, "interface");
			unlink_f(u_sys_dir, "interface");
		}
		unlink_f(u_slot_dir, u_func);
		unlink_f(u_sys_dir, u_func);
		// try to remove directories - succesfull if empty
		rmdir(u_slot_dir);
		rmdir(u_sys_dir);
	}
	return 0;
}
