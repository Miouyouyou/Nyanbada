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

#include <libdrm/drm.h>

#include <string.h>

#define LOG(msg, ...) \
	fprintf(\
		stderr, "[%s (%s:%d)] "msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)

int create_dumb_buffer
(int drm_fd, struct drm_mode_create_dumb * __restrict create_request)
{
	return ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, create_request);
}


int main() {

	int ret = 0;
	
	int vpu_fd = open("/dev/vpu-service", O_RDWR);

	if (vpu_fd < 0)
		goto program_end;

	ioctl(vpu_fd, MYY_IOCTL_HELLO, NULL);
	ioctl(vpu_fd, MYY_IOCTL_TEST_FD_PASSING, vpu_fd);

	int drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	struct drm_mode_create_dumb dumb_buffer_to_export = {
		.width  = 1440,
		.height = 900,
		.bpp    = 32
	};

	ret = create_dumb_buffer(drm_fd, &dumb_buffer_to_export);
	if (ret < 0) {
		LOG("Can't allocate dumb buffer. %m\n");
		goto dumb_buffer_not_allocated;
	}

	LOG("Buffer size : %llu\n", dumb_buffer_to_export.size);

	struct drm_prime_handle export_request = {
		.handle = dumb_buffer_to_export.handle
	};

	ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &export_request);

	if (ret < 0 || export_request.fd < 0) {
		LOG(
			"Can't export dumb buffer. %s (FD : %d)\n",
			strerror(ret),
			export_request.fd
 		);
		goto dumb_buffer_not_exported;
	}

	uint32_t exported_fd = export_request.fd;

	ioctl(vpu_fd, MYY_IOCTL_TEST_DMA_FD_PASSING, exported_fd);

dumb_buffer_not_exported: {
		struct drm_mode_destroy_dumb const destroy_dumb_request = {
			.handle = dumb_buffer_to_export.handle
		};
		ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb_request);
	}
dumb_buffer_not_allocated:
	close(drm_fd);
drm_fd_not_open:
	close(vpu_fd);
program_end:
	return ret;
}
