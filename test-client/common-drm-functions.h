#ifndef __MYY_COMMON_DRM_FUNCTIONS_H
#define __MYY_COMMON_DRM_FUNCTIONS_H 1

#include <stdint.h>
#include <libdrm/drm.h>

int allocate_drm_dumb_buffer
(int const drm_fd,
 uint32_t width, uint32_t height, uint32_t bpp,
 uint32_t * __restrict const dumb_gem_handle);

void free_drm_dumb_buffer
(int const drm_fd, uint32_t dumb_handle);

int drm_open_node
(char const * __restrict dev_node_path);

int drm_convert_buffer_handle_to_prime_fd
(int drm_fd, int gem_handle);

uint_fast8_t drm_node_support_dumb_buffers
(int const drm_fd);

#endif

