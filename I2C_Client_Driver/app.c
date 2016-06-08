#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include <linux/i2c-dev.h>
int main()
{
    int fd;
    long int i=160000;
    char buffer[20];
    char *buff;
    char address = 0x40;
    char *msg = "adsadsdasdas";

    buff = (char *) malloc(sizeof(char)*50);
    sprintf(buff, "%x", address);
//    printf("write message is ==%saaaa\n", buff);
    strcat(buff, msg);
//    printf("write message is =%s\n", buff);
    fd = open("/dev/my_eeprom", O_RDWR);

    if (fd < 0)
        perror("Unable to open the device ");
    else
        printf("File opened Sucessfully %d\n", fd);

    read(fd, buffer, 7);
    printf("Read message is %s\n", buffer);
    write(fd, buff, sizeof(buff)-1);
    printf("write message is %s\n", buff);
    // delay
    while (i-- > 0); 
    close(fd);
    return 0; 
}
