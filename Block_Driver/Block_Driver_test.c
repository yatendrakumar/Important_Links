#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#define XFER_SIZE         1*1000*512
#define BYTES_PER_BLOCK   512

int main (){
    int fd, size;
    unsigned char *buf;
    off_t offset = 0;

    buf = (char *)malloc(XFER_SIZE);
    memset(buf, 0x30, XFER_SIZE);

    if((fd = open("/dev/yatendra", O_RDWR)) < 0){
        printf("%s\n", strerror(errno));
        return errno;
    }
    printf("BLKGETSIZE  = %d", BLKGETSIZE);
    if(ioctl(fd, BLKGETSIZE, &size) == -1){
        printf("%s\n", strerror(errno));
        return errno;
    }else{
        printf("total block count: %d\n", size);
        printf("total bytes count: %d\n", (size * BYTES_PER_BLOCK));
    }

    if((write (fd, buf, XFER_SIZE)) <= 0){
        printf("%s\n", strerror(errno));
        return errno;
    }
    printf ("wrote %d bytes at offset %d\n", XFER_SIZE, offset);
    close (fd);
    if((fd = open("/dev/yatendra", O_RDWR)) < 0){
        printf("%s\n", strerror(errno));
        return errno;
    }
    if((read(fd, buf, XFER_SIZE)) <= 0){
        printf("%s\n", strerror(errno));
        return errno;
    }
    printf ("read %d bytes at offset %d\n", XFER_SIZE, offset);
    /*
       offset = (offset + 65536);
       if((lseek (fd, offset, SEEK_SET)) != offset){
       printf("%s\n", strerror(errno));
       return errno;
       }
       printf ("seeked to offset %d\n", offset);

       if((read(fd, buf, XFER_SIZE)) <= 0){
       printf("%s\n", strerror(errno));
       return errno;
       }
       printf ("read %d bytes at offset %d\n", XFER_SIZE, offset);
     */
    close (fd);

    return 0;
}
