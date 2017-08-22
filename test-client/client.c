#include <sys/ioctl.h>

// fprintf
#include <stdio.h>

// close
#include <unistd.h>

#include <stdint.h>

// struct drm_mode_create_dumb
#include <libdrm/drm.h>

// drmModeGetResources -- Does not depend on X11
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <errno.h>

#include <sys/mman.h>

// strerror
#include <string.h>

// rand
#include <stdlib.h>

#include "common-drm-functions.h"



// -- COMPILE, RUN BUT STILL REQUIRE TESTING
struct myy_drm_internal_structures {
	drmModeRes       * __restrict drm_resources;
	drmModeConnector * __restrict valid_connector;
	drmModeModeInfo  * __restrict chosen_resolution;
	drmModeEncoder   * __restrict screen_encoder;
};
struct myy_drm_frame_buffer {
	uint8_t * buffer;
	uint32_t id;
	struct drm_mode_create_dumb metadata;
	drmModeFB * __restrict underlying_object;
	struct myy_drm_internal_structures related_structures;
};

drmModeModeInfo * choose_preferred_resolution_on_connector
(drmModeConnector * __restrict const connector)
{
	drmModeModeInfo * best_resolution = NULL;

	for (int_fast32_t m = 0; m < connector->count_modes; m++) {
		drmModeModeInfo * __restrict tested_resolution =
			&connector->modes[m];
		if (tested_resolution->type & DRM_MODE_TYPE_PREFERRED) {
			best_resolution = tested_resolution;
			break;
		}
	}

	return best_resolution;
}
int drm_init_display
(int const drm_fd,
 struct myy_drm_frame_buffer * __restrict const myy_fb)
{
	/* DRM is based on the fact that you can connect multiple screens,
	 * on multiple different connectors, which have, of course, multiple
	 * encoders that transform applications framebuffer represented in
	 * XRGB8888 into something the selected screen comprehend, before
	 * sending it this framebuffer for display.
	 * 
	 * The default selection system is simple :
	 * - We try to find the first connected screen that accept our
	 *   lowest resolution (1280x720 32BPP)
	 */
	drmModeRes         * __restrict drm_resources;
	drmModeConnector   * __restrict valid_connector   = NULL;
	drmModeModeInfo    * __restrict chosen_resolution = NULL;
	drmModeEncoder     * __restrict screen_encoder    = NULL;
	int ret = 0;

	/* Let's see what we can use through this drm node */
	drm_resources = drmModeGetResources(drm_fd);

	for (int_fast32_t c = 0; c < drm_resources->count_connectors; c++) {
		valid_connector =
			drmModeGetConnector(drm_fd, drm_resources->connectors[c]);

		if (valid_connector->connection == DRM_MODE_CONNECTED)
			break;

		drmModeFreeConnector(valid_connector);
		valid_connector = NULL;
	}

	if (!valid_connector) {
		/* Then there was no connectors,
		 * or no connector were connected */
		LOG("No connectors or no connected connectors found...\n");
		ret = -ENOLINK;
		goto no_valid_connector;
	}

	chosen_resolution =
		choose_preferred_resolution_on_connector(valid_connector);

	if (!chosen_resolution) {
		LOG(
			"No preferred resolution on the selected connector %u ?\n",
			valid_connector->connector_id
		);
		ret = -ENAVAIL;
		goto no_valid_resolution;
	}

	screen_encoder =
		drmModeGetEncoder(drm_fd, valid_connector->encoder_id);

	if (!screen_encoder) {
		LOG(
			"Could not retrieve the encoder for mode %s on connector %u.\n",
			chosen_resolution->name,
			valid_connector->connector_id
		);
		ret = -ENOLINK;
		goto could_not_retrieve_encoder;
	}

	print_drmModeResources(drm_resources);
	print_drmModeModeInfo(chosen_resolution);

	myy_fb->related_structures.drm_resources     = drm_resources;
	myy_fb->related_structures.valid_connector   = valid_connector;
	myy_fb->related_structures.chosen_resolution = chosen_resolution;
	myy_fb->related_structures.screen_encoder    = screen_encoder;

	myy_fb->metadata.width  = chosen_resolution->hdisplay;
	myy_fb->metadata.height = chosen_resolution->vdisplay;
	return ret;

could_not_retrieve_encoder:
	drmModeFreeModeInfo(chosen_resolution);
no_valid_resolution:
	drmModeFreeConnector(valid_connector);
no_valid_connector:
	return ret;
}



void drm_deinit_display()
{
	
}

void * mmap_dumb_buffer
(int drm_fd,
 struct drm_mode_create_dumb * __restrict const dumb_buffer)
{
	struct drm_mode_map_dumb map_request = {
		.handle = dumb_buffer->handle,
		.pad    = 0,
		.offset = 0
	};

	void * mmapped_address = NULL;

	if (ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_request) < 0)
		goto dumb_buffer_mapping_request_denied;

	mmapped_address = mmap(
		0, dumb_buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		drm_fd, map_request.offset);

dumb_buffer_mapping_request_denied:
	return mmapped_address;

}

uint32_t drm_crtc_get_current
(struct myy_drm_frame_buffer * __restrict const myy_fb)
{
	return myy_fb->related_structures.screen_encoder->crtc_id;
}

// -- TESTED FUNCTIONS
int main() {
	char const * __restrict const drm_hardware_dev_node_path =
		"/dev/dri/card0";
	const uint32_t fb_depth = 24;
	struct myy_drm_frame_buffer myy_drm_fb = {0};
	int const drm_fd   = drm_open_node(drm_hardware_dev_node_path);
	int prime_fd = -1;
	int ret = 0;
	uint32_t current_crtc_id = 0;
	drmModeCrtc * this_program_crtc = NULL;
	drmModeCrtc * crtc_to_restore = NULL;

	if (drm_fd < 0) {
		/* %m is actually a GLIBC specific format for strerror(errno) */
		LOG(
			"Could not open %s with Read-Write rights : %m\n",
			drm_hardware_dev_node_path
		);
		goto could_not_open_drm_fd;
	}

	if (!drm_node_support_dumb_buffers(drm_fd)) {
		LOG(
			"Looks like %s do not support Dumb Buffers...\n",
			drm_hardware_dev_node_path
		);
		goto dumb_buffers_not_supported;
	}

	if (drm_init_display(drm_fd, &myy_drm_fb)) {
		LOG("Something went wrong when initialiasing the display\n");
		goto could_not_init_display;
	}

	myy_drm_fb.metadata.bpp = 32;
	ret = allocate_drm_dumb_buffer(drm_fd, &myy_drm_fb.metadata);

	if (ret < 0) {
		LOG(
			"Could not allocate a %ux%u@%ubpp frame buffer... %m\n",
			myy_drm_fb.metadata.width,
			myy_drm_fb.metadata.height,
			myy_drm_fb.metadata.bpp
		);
		goto could_not_allocate_drm_dumb_buffer;
	}

	ret = drmModeAddFB(
		drm_fd,
		myy_drm_fb.metadata.width, myy_drm_fb.metadata.height,
		fb_depth, myy_drm_fb.metadata.bpp, myy_drm_fb.metadata.pitch,
		myy_drm_fb.metadata.handle, &myy_drm_fb.id
	);

	if (ret) {
		LOG(
			"Could not add a framebuffer using drmModeAddFB : %s\n",
			strerror(ret)
		);
		goto could_not_add_frame_buffer;
	}
	current_crtc_id = drm_crtc_get_current(&myy_drm_fb);

	if (!current_crtc_id) {
		LOG("The retrieved encoder has no CRTC attached... ?\n");
		goto could_not_retrieve_current_crtc;
	}

	crtc_to_restore = drmModeGetCrtc(drm_fd, current_crtc_id);

	if (!crtc_to_restore) {
		LOG("Could not retrieve the current CRTC with a valid ID !\n");
		goto could_not_retrieve_current_crtc;
	}

	print_crtc(crtc_to_restore);

	ret = drmModeSetCrtc(
		drm_fd, current_crtc_id, myy_drm_fb.id, 0, 0,
		&myy_drm_fb.related_structures.valid_connector->connector_id,
		1, myy_drm_fb.related_structures.chosen_resolution);

	/*prime_fd = drm_convert_buffer_handle_to_prime_fd(
		drm_fd, myy_drm_fb.metadata.handle);
	if (prime_fd < 0) {
		LOG("Unable to convert the GEM handle to a DRM Handle : %m\n");
		goto could_not_export_dumb_buffer;
	}

	LOG("Got a Prime buffer FD : %d ! Yay !\n", prime_fd);*/

	myy_drm_fb.buffer = mmap_dumb_buffer(drm_fd, &myy_drm_fb.metadata);

	if (!myy_drm_fb.buffer || myy_drm_fb.buffer == MAP_FAILED) {
		LOG("Could not map buffer exported through PRIME : %m\n");
		goto could_not_map_buffer;
	}
	/* Map the buffer and write in it */

	LOG(
		"Buffer address : %p\n"
		"Buffer ID      : %u\n"
		"Buffer width   : %u\n"
		"Buffer height  : %u\n"
		"Buffer bpp     : %u\n"
		"Buffer size    : %llu\n"
		"Buffer stride  : %u\n",
		myy_drm_fb.buffer,
		myy_drm_fb.id,
		myy_drm_fb.metadata.width,
		myy_drm_fb.metadata.height,
		myy_drm_fb.metadata.bpp,
		myy_drm_fb.metadata.size,
		myy_drm_fb.metadata.pitch);
	for (uint_fast64_t pixel = 0, size = myy_drm_fb.metadata.size;
	     pixel < size; pixel++) {
		myy_drm_fb.buffer[pixel] = rand();
	}

	getc(stdin);
	munmap(myy_drm_fb.buffer, myy_drm_fb.metadata.size);

could_not_map_buffer:
//could_not_export_dumb_buffer:
	free_drm_dumb_buffer(drm_fd, myy_drm_fb.metadata.handle);
	drmModeSetCrtc(
		drm_fd, crtc_to_restore->crtc_id,
		crtc_to_restore->buffer_id, 0, 0,
		&myy_drm_fb.related_structures.valid_connector->connector_id,
		1, &crtc_to_restore->mode);
could_not_retrieve_current_crtc:
could_not_add_frame_buffer:
	drmModeRmFB(drm_fd, myy_drm_fb.id);
could_not_allocate_drm_dumb_buffer:
dumb_buffers_not_supported:
could_not_init_display:
	close(drm_fd);
could_not_open_drm_fd:
	return 0;
}
