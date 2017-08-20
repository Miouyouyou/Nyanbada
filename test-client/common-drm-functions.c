#include "common-drm-functions.h"

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// ioctl...
#include <sys/ioctl.h>

// O_CLOEXEC
#include <asm-generic/fcntl.h>

// errno
#include <errno.h>

int allocate_drm_dumb_buffer
(int const drm_fd,
 struct drm_mode_create_dumb * __restrict const metadata)
{
	int ret = 0;

	if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, metadata) < 0)
		ret = -errno;

	return ret;
}

void free_drm_dumb_buffer
(int const drm_fd, uint32_t const dumb_handle)
{
	struct drm_mode_destroy_dumb request_destroy = {
		.handle = dumb_handle
	};

	ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &request_destroy);
}

int drm_open_node(char const * __restrict const dev_node_path) {
	return open(dev_node_path, O_RDWR | O_CLOEXEC);
}

int drm_convert_buffer_handle_to_prime_fd
(int const drm_fd, int const gem_handle)
{
	struct drm_prime_handle prime_structure = {
		.handle = gem_handle,
		.flags  = DRM_CLOEXEC,
		.fd     = -1
	};

	ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_structure);

	return prime_structure.fd;
}

uint_fast8_t drm_node_support_dumb_buffers(int const drm_fd)
{

	struct drm_get_cap support_dumb = {
		.capability = DRM_CAP_DUMB_BUFFER,
		.value      = 0
	};

	int ret = ioctl(drm_fd, DRM_IOCTL_GET_CAP, &support_dumb);

	return ret >= 0 && support_dumb.value;
}
