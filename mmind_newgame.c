#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "mastermind_ioctl.h"

void main(int argc, char *argv[]){

	param_t parampa;
	strncpy(parampa.buffer, argv[1], strlen(argv[1]));
	int fd = open("/dev/mastermind", O_RDWR);
	int status = ioctl(fd, MMIND_NEWGAME, &parampa);
}
