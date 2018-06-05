#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "oled_ioctl.h"

int main(){
	char* file_name = "/dev/write_oled";
int fd; 
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

fd = open(file_name,O_WRONLY|O_CREAT|O_TRUNC,mode);

	if(fd==-1){
		printf("ERROR");
		return -1;
	}

	ioctl(fd,IO_INV);
	close(fd);

	return 0;

}
