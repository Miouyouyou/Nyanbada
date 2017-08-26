#include <sys/ioctl.h>

#include <errno.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// fprintf
#include <stdio.h>

// close
#include <unistd.h>

#include <stdint.h>

#include "../include/myy_ioctl.h"

#define LOG(msg, ...) \
	fprintf(\
		stderr, "[%s (%s:%d)] "msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)


int main() {

	int vpu_fd = open("/dev/vpu-service", O_RDWR);

	if (vpu_fd < 0)
		goto program_end;

	ioctl(vpu_fd, MYY_IOCTL_HELLO, NULL);
	close(vpu_fd);

program_end:
	return 0;
}
