#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "mastermind_ioctl.h"

void main(int argc, char *argv[]){

	int fd = open("/dev/mastermind", O_RDWR);
	int status = ioctl(fd, MMIND_ENDGAME, 0);
}
