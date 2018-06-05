#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h> 
#include "oled_ioctl.h"

int main(int argc, char **argv){
	char* file_name = "/dev/write_oled";
int fd; 
int number;
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

fd = open(file_name,O_WRONLY|O_CREAT|O_TRUNC,mode);
	if(argc>2){
		printf("ERROR");
	 	return -1;	
	}
	//number = (uint8_t)*(argv+1);
	number = atoi(*(argv+1));
	if(fd==-1){
		printf("ERROR");
		return -1;
	}
	printf("brill: %d\n",number);

	ioctl(fd,IO_CON,&number);
	close(fd);

	return 0;

}
