#ifndef __MYY_COMMON_DRM_FUNCTIONS_H
#define __MYY_COMMON_DRM_FUNCTIONS_H 1

#include <stdint.h>
#include <libdrm/drm.h>

// drmMode... drm structures names written in CamelCase are from there
#include <xf86drm.h>
#include <xf86drmMode.h>

// LOG
#include <stdio.h>

#define LOG(msg, ...) \
	fprintf(\
		stderr, "\n[%s (%s:%d)]\n"msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)

int allocate_drm_dumb_buffer
(int drm_fd,
 struct drm_mode_create_dumb * __restrict metadata);

void free_drm_dumb_buffer
(int const drm_fd, uint32_t dumb_handle);

int drm_open_node
(char const * __restrict dev_node_path);

int drm_convert_buffer_handle_to_prime_fd
(int drm_fd, int gem_handle);

uint_fast8_t drm_node_support_dumb_buffers
(int const drm_fd);

void print_drmModeResources
(drmModeRes const * __restrict const res);

void print_drmModeModeInfo
(drmModeModeInfo const * __restrict const crtc);

void print_crtc
(drmModeCrtc const * __restrict const crtc);

#endif

