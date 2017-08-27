#include <stdio.h>
#include <libdrm/drm.h>

#include <stdint.h>

#include <sys/mman.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/ioctl.h>
#include <errno.h>

#define LOG(msg, ...) \
	fprintf(\
		stderr, "\n[%s (%s:%d)]\n"msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)

#define ALIGN_ON_POW2(n, align) ((n + align - 1) & ~(align - 1))

// FAIL with PERMISSION DENIED
int main()
{
	const uint32_t width = 1440, height = 900;

	int ret;

	int const drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		LOG("Could not open /dev/dri/card0 : %m\n");
		goto could_not_open_drm_fd;
	}
	
	struct drm_mode_create_dumb create_request = {
		.width  = width,
		.height = height,
		.bpp    = 32
	};
	ret = ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_request);

	if (ret) {
		LOG(
			"Dumb Buffer Object Allocation request of %ux%u@%u failed : %s\n",
			create_request.width, create_request.height,
			create_request.bpp,
			strerror(ret)
		);
		goto could_not_allocate_buffer;
	}

	struct drm_prime_handle prime_request = {
		.handle = create_request.handle,
		.flags  = DRM_CLOEXEC | DRM_RDWR,
		.fd     = -1
	};

	ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);
	int const dma_buf_fd = prime_request.fd;

	if (ret || dma_buf_fd < 0) {
		LOG(
			"Could not export buffer : %s (%d) - FD : %d\n",
			strerror(ret), ret,
			dma_buf_fd
 		);
		goto could_not_export_buffer;
	}

	void * buffer = mmap(
		0, create_request.size,	PROT_READ | PROT_WRITE, MAP_SHARED,
		dma_buf_fd, 0);
	ret = errno;

	if (buffer == NULL || buffer == MAP_FAILED) {
		LOG(
			"Could not map buffer exported through PRIME : %s (%d)\n"
			"Buffer : %p\n",
			strerror(ret), ret,
			buffer
		);
		goto could_not_map_buffer;
	}

	LOG("Buffer mapped !\n");

	munmap(buffer, create_request.size);

could_not_map_buffer:
could_not_export_buffer:
	// very ugly but will do for an example...
	{
		struct drm_mode_destroy_dumb destroy_request = {
			.handle = create_request.handle
		};
		
		ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_request);
	}
could_not_allocate_buffer:
	close(drm_fd);
could_not_open_drm_fd:
	return 0;
}

