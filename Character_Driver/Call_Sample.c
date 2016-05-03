#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define BUFFER_LENGTH 256               ///< The buffer length
static char receive[BUFFER_LENGTH];     ///< The receive buffer

#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)

int main()
{
    int ret, fd = -1;
    char stringToSend[BUFFER_LENGTH];

    printf("Opening /dev/mychar file and performing operations....\n");
    fd = open("/dev/mychar", O_RDWR);             // Open the device with read/write access
    if (fd < 0){
        perror("Failed to open the device:");
        return errno;
    }
	int i = 5;
    while ( i ) {
        printf("Performing IOCTL operations\n");
        printf("Enter characters to send to the kernel module:\n");
        scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
        if(ioctl(fd, WRITE_IOCTL, stringToSend) < 0)
            perror("Fail write ioctl:");
        if(ioctl(fd, READ_IOCTL, receive) < 0)
            perror("Fail read ioctl:");

        printf("message: %s\n", receive);
        i--;
    }
    printf("Performing read and write operations\n");
    printf("Enter characters to send to the kernel module:\n");
    scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)

    ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string 
    if (ret < 0)
    {
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Press ENTER to read back from the device...\n");
    getchar();

    printf("Reading from the device...\n");
    ret = read(fd, receive, BUFFER_LENGTH);        // Read the response  
    if (ret < 0)
    {
        perror("Failed to read the message from the device.");
        return errno;
    }

    printf("The received message is: [%s]\n", receive);
    printf("End of the program\n");
    return 0;
}
