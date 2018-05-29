#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "oled_ioctl.h"

int main(){
	char* file_name = "/dev/write_oled";
int fd; 
    fd = open(file_name, O_RDWR);

	if(fd==-1){
		printf("ERROR");
		return -1;
	}

	ioctl(fd,IO_CLEAN);
	close(fd);

	return 0;

}
