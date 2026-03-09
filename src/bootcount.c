
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define bootcount_magic 0xbc
#define offset 0
const char* filename = "/run/unipi-plc/by-sys/bootcount/nvmem";

int writeboot(int bootcount)
{
    int fd;
    off_t position;
    uint16_t wbootcount;
    int ret=0;

    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error open('%s'): %s\n", filename, strerror(errno));
        return -2;
    }
    position = lseek(fd, offset, SEEK_SET);
    if (position == offset) {
        wbootcount = (bootcount & 0xff) | (bootcount_magic << 8);
        ret = write(fd, &wbootcount, sizeof(wbootcount));
        if (ret != sizeof(wbootcount))
            ret = -2;
    }
    close(fd);
    return ret;
}

int readboot(void)
{
    int fd;
    off_t position;
    uint16_t bootcount;
    int ret;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error open('%s'): %s\n", filename, strerror(errno));
        return -2;
    }
    position = lseek(fd, offset, SEEK_SET);
    if (position == offset) {
        ret = read(fd, &bootcount, sizeof(bootcount));
        if (ret == sizeof(bootcount)) {
            if ((bootcount >> 8) == bootcount_magic) {
                ret = bootcount & 0xff;
            } else {
                fprintf(stderr, "Invalid magic value(read=0x%02x required=0x%02x). bootcount=%d\n",
                       bootcount >>8, bootcount_magic,bootcount&0xff);
                ret = -1;
            }
        } else if (ret < 0) {
            fprintf(stderr, "Error read('%s'): %s\n", filename, strerror(errno));
            ret = -2;
        } else {
            fprintf(stderr, "Cannot read bootcount(required size=%ld, read size=%d) from '%s'\n", sizeof(bootcount), ret, filename);
            ret = -1;
        }
    } else {
        fprintf(stderr, "Cannot seek to position %d file '%s': %s\n", offset, filename, strerror(errno));
        ret = -1;
    }
    close(fd);
    return ret;
}

int main(int argc, char** argv)
{
    int mode = 0;
    if (argc > 1) {
        if (strcmp(argv[1], "clear") == 0) mode = 1;
        if (strcmp(argv[1], "set") == 0) mode = 99;
    }
    if (mode == 99 && argc > 2) {
        if (strcmp(argv[2], "a") == 0) mode = 100;
        if (strcmp(argv[2], "b") == 0) mode = 101;
    }
    int bootcount = readboot();
    if (bootcount < 0)
        return -bootcount;
    if (mode==0) {
        printf("bootcount=%d\n", bootcount);
    } else {
        writeboot(mode==1?0:mode);
        printf("bootcount: %d -> %d\n", bootcount, readboot());
    }
}
