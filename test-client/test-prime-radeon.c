#include <stdio.h>
#include <libdrm/drm.h>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>

#include <stdint.h>

#include <sys/mman.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/ioctl.h>

#define LOG(msg, ...) \
	fprintf(\
		stderr, "\n[%s (%s:%d)]\n"msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)

#define ALIGN_ON_POW2(n, align) ((n + align - 1) & ~(align - 1))

/* 
 * FAIL with -ENOSYS
 * The reason being :
 * gem_prime_mmap is not implemented on amdgpu driver.
 */
int main()
{
	const uint32_t width = 1440, height = 900;
	const size_t buffer_size = ALIGN_ON_POW2(width*height*4, 4096);

	int ret;

	int const drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		LOG("Could not open /dev/dri/card0 : %m\n");
		goto could_not_open_drm_fd;
	}
	
	union drm_amdgpu_gem_create create_request = {
		.in = {
			.bo_size   = buffer_size,
			.alignment = 4096,
			.domains   = 
				AMDGPU_GEM_DOMAIN_CPU | AMDGPU_GEM_DOMAIN_GTT |
				AMDGPU_GEM_DOMAIN_VRAM,
			.domain_flags = AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED
		}
	};
	ret = ioctl(drm_fd, DRM_IOCTL_AMDGPU_GEM_CREATE, &create_request);

	if (ret) {
		LOG(
			"Buffer Object Allocation request of %lu bytes failed : %s\n",
			buffer_size,
			strerror(ret)
		);
		goto could_not_allocate_buffer;
	}

	struct drm_prime_handle prime_request = {
		.handle = create_request.out.handle,
		.flags  = DRM_CLOEXEC | DRM_RDWR,
		.fd     = -1
	};

	ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);
	int const dma_buf_fd = prime_request.fd;

	if (ret || dma_buf_fd < 0) {
		LOG("Could not export buffer... ?\n");
		goto could_not_export_buffer;
	}

	void * buffer = mmap(
		0, buffer_size,	PROT_READ | PROT_WRITE, MAP_SHARED,
		dma_buf_fd, 0);

	if (buffer == NULL || buffer == MAP_FAILED) {
		LOG("Could not map buffer exported through PRIME : %m\n");
		goto could_not_map_buffer;
	}

	LOG("Buffer mapped !\n");

	munmap(buffer, buffer_size);

could_not_map_buffer:
could_not_export_buffer:
could_not_allocate_buffer:
	close(drm_fd);
could_not_open_drm_fd:
	return 0;
}

