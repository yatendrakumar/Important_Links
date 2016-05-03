#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


    int
main(int argc, char *argv[])
{
    int sim_fd;
    int x, y;
    char buffer[10];

    /* Open the sysfs coordinate node */
    sim_fd = open("/sys/devices/platform/virtual_mouse/coordinates", O_RDWR);
    if (sim_fd < 0) {
        perror("Couldn't open virtual_mouse coordinate file\n");
        exit(-1);
    }

    while (1) {
        /* Generate random relative coordinates */
        x = random()%100;
        y = random()%100;
//        if (x%2) 
//            x = -x; 
//        if (y%2) 
//            y = -y;

        /* Convey simulated coordinates to the virtual mouse driver */
        sprintf(buffer, "%d %d %d", x, y, 0);
        write(sim_fd, buffer, strlen(buffer));
        printf("Inside while loop\n");
        read(sim_fd, buffer, strlen(buffer));
        printf("Buffer = %s\n", buffer);
        fsync(sim_fd);
        sleep(1);
    }

    close(sim_fd);
}
