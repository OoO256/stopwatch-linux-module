#include <asm/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAJOR_NUMBER 242
#define DEVICE "/dev/stopwatch"


int main(int argc, char** argv){


    // Device driver open
    int dev = open(DEVICE, O_RDWR);
    if (dev < 0){
        printf("Device open error : %s\n", DEVICE);
        exit(1);
    }
    else { 
        printf("< Device has been detected > \n"); 
    }
    
    char buf[2] = {0,};
    write(dev, buf, 2);
    close(dev);
    return 0;
}