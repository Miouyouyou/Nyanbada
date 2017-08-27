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
	amdgpu_device_handle device_handle;
	uint32_t major_version, minor_version;
	int ret;

	int const drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		LOG("Could not open /dev/dri/card0 : %m\n");
		goto could_not_open_drm_fd;
	}
	ret = amdgpu_device_initialize(
		drm_fd, &major_version, &minor_version, &device_handle
	);

	if (ret) {
		LOG("Could not initialize the GPU ??\n");
		goto could_not_initialize_gpu;
	}
	LOG(
		"AMDGPU Device initialization :\n"
		"	Major version : %d\n"
		"	Minor version : %d\n",
		 major_version, minor_version
	);

	size_t buffer_size = ALIGN_ON_POW2(width*height*4, 512);
	struct amdgpu_bo_alloc_request request = {
		.alloc_size     = buffer_size,
		.phys_alignment = 512,
		.preferred_heap = AMDGPU_GEM_DOMAIN_GTT,
	};

	amdgpu_bo_handle bo_handle;

	ret = amdgpu_bo_alloc(device_handle, &request, &bo_handle);

	if (ret) {
		LOG(
			"Buffer Object Allocation request of %lu bytes failed : %s\n",
			buffer_size,
			strerror(ret)
		);
		goto could_not_allocate_buffer;
	}

	int dma_buf_fd;
	ret = amdgpu_bo_export(
		bo_handle, amdgpu_bo_handle_type_dma_buf_fd, &dma_buf_fd
	);

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
	amdgpu_bo_free(bo_handle);
could_not_allocate_buffer:
	amdgpu_device_deinitialize(device_handle);
could_not_initialize_gpu:
	close(drm_fd);
could_not_open_drm_fd:
	return 0;
}

