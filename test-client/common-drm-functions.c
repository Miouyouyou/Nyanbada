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

void print_drmModeResources
(drmModeRes const * __restrict const res)
{
	LOG(
		"Framebuffers count   : %d\n"
		"Framebuffers address : %p\n"
		"CRTC count           : %d\n"
		"CRTC address         : %p\n"
		"Connectors count     : %d\n"
		"Connectors address   : %p\n"
		"Encoders count       : %d\n"
		"Encoders address     : %p\n"
		"Width  Min, Max      : %u, %u\n"
		"Height Min, Max      : %u, %u\n",
		 res->count_fbs,        res->fbs,
		 res->count_crtcs,      res->crtcs,
		 res->count_connectors, res->connectors,
		 res->count_encoders,   res->encoders,
		 res->min_width,  res->max_width,
		 res->min_height, res->max_height
	);
}

void print_drmModeModeInfo
(drmModeModeInfo const * __restrict const crtc)
{
	LOG(
		"Clock : %u\n"
		"Horizontal\n"
		"  Display     : %u\n"
		"  Sync start  : %u\n"
		"  Sync End    : %u\n"
		"  Total       : %u\n"
		"  Skew        : %u\n"
		"Vertical\n"
		"  Display     : %u\n"
		"  Sync start  : %u\n"
		"  Sync End    : %u\n"
		"  Total       : %u\n"
		"  Scan        : %u\n"
		"Refresh Rate  : %u\n"
		"Flags         : %x\n"
		"Type          : %x\n"
		"Name          : %s\n",
		 crtc->clock,
		 crtc->hdisplay, crtc->hsync_start, crtc->hsync_end,
		 crtc->htotal, crtc->hskew,
		 crtc->vdisplay, crtc->vsync_start, crtc->vsync_end,
		 crtc->vtotal, crtc->vscan,
		 crtc->vrefresh,
		 crtc->flags,
		 crtc->type,
		 crtc->name
	);
}

void print_crtc
(drmModeCrtc const * __restrict const crtc)
{
	LOG(
		"CRTC ID            : %u\n"
		"CRTC Buffer ID     : %u\n"
		"CRTC X, Y          : %u, %u\n"
		"CRTC Width, Height : %u, %u\n"
		"CRTC Mode Valid    : %d\n",
		 crtc->crtc_id,
		 crtc->buffer_id,
		 crtc->x,
		 crtc->y,
		 crtc->width,
		 crtc->height,
		 crtc->mode_valid
	);
	print_drmModeModeInfo(&crtc->mode);
	LOG("CRTC Gamma Size   : %u\n", crtc->gamma_size);
}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
