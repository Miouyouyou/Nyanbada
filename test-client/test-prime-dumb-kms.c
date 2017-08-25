
#include <stdio.h>
#include <libdrm/drm.h>

#include <stdint.h>

#include <sys/mman.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// O_CLOEXEC
//#include <asm-generic/fcntl.h>

#include <unistd.h>

#include <sys/ioctl.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

// rand
#include <stdlib.h>

#define LOG(msg, ...) \
	fprintf(\
		stderr, "\n[%s (%s:%d)]\n"msg,\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__ \
	)

#define ALIGN_ON_POW2(n, align) ((n + align - 1) & ~(align - 1))

// Works on Rockchip systems but fail with ENOSYS on AMDGPU
int main()
{

	/* DRM is based on the fact that you can connect multiple screens,
	 * on multiple different connectors which have, of course, multiple
	 * encoders that transform CRTC (The screen final buffer where all
	 * the framebuffers are blended together) represented in XRGB8888 (or
	 * similar) into something the selected screen comprehend.
	 * (Think XRGB8888 to DVI-D format for example)
	 * 
	 * The default selection system is simple :
	 * - We try to find the first connected screen and choose its
	 *   preferred resolution.
	 */
	drmModeRes         * __restrict drm_resources;
	drmModeConnector   * __restrict valid_connector   = NULL;
	drmModeModeInfo    * __restrict chosen_resolution = NULL;
	drmModeEncoder     * __restrict screen_encoder    = NULL;

	int ret = 0;

	/* Open the DRM device node and get a File Descriptor */
	int const drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	if (drm_fd < 0) {
		LOG("Could not open /dev/dri/card0 : %m\n");
		goto could_not_open_drm_fd;
	}

	/* Let's see what we can use through this drm node */
	drm_resources = drmModeGetResources(drm_fd);

	/* Get a valid connector. A valid connector is one that's connected */
	for (int_fast32_t c = 0; c < drm_resources->count_connectors; c++) {
		valid_connector =
			drmModeGetConnector(drm_fd, drm_resources->connectors[c]);

		if (valid_connector->connection == DRM_MODE_CONNECTED)
			break;

		drmModeFreeConnector(valid_connector);
		valid_connector = NULL;
	}

	/* Bail out if nothing was connected */
	if (!valid_connector) {
		/* Then there was no connectors,
		 * or no connector were connected */
		LOG("No connectors or no connected connectors found...\n");
		ret = -ENOLINK;
		goto no_valid_connector;
	}

	/* Get the preferred resolution */
	for (int_fast32_t m = 0; m < valid_connector->count_modes; m++) {
		drmModeModeInfo * __restrict tested_resolution =
			&valid_connector->modes[m];
		if (tested_resolution->type & DRM_MODE_TYPE_PREFERRED) {
			chosen_resolution = tested_resolution;
			break;
		}
	}

	/* Bail out if there's no such thing as a "preferred resolution" */
	if (!chosen_resolution) {
		LOG(
			"No preferred resolution on the selected connector %u ?\n",
			valid_connector->connector_id
		);
		ret = -ENAVAIL;
		goto no_valid_resolution;
	}

	/* Get an encoder that will transform our CRTC data into something
	 * the screen comprehend natively, through the chosen connector */
	screen_encoder =
		drmModeGetEncoder(drm_fd, valid_connector->encoder_id);

	/* If there's no screen encoder through the chosen connector, bail
	 * out quickly. */
	if (!screen_encoder) {
		LOG(
			"Could not retrieve the encoder for mode %s on connector %u.\n",
			chosen_resolution->name,
			valid_connector->connector_id
		);
		ret = -ENOLINK;
		goto could_not_retrieve_encoder;
	}

	/* We're almost done with KMS. We'll now allocate a "dumb" buffer on
	 * the GPU, and use it as a "frame buffer", that is something that
	 * will be read and displayed on screen (the CRTC to be exact) */

	/* Request a dumb buffer */
	struct drm_mode_create_dumb create_request = {
		.width  = chosen_resolution->hdisplay,
		.height = chosen_resolution->vdisplay,
		.bpp    = 32
	};
	ret = ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_request);

	/* Bail out if we could not allocate a dumb buffer */
	if (ret) {
		LOG(
			"Dumb Buffer Object Allocation request of %ux%u@%u failed : %s\n",
			create_request.width, create_request.height,
			create_request.bpp,
			strerror(ret)
		);
		goto could_not_allocate_buffer;
	}

	/* Create a framebuffer, using the old method.
	 * 
	 * A new method exist, drmModeAddFB2 : Return of the Revengeance !
	 * 
	 * Jokes aside, this other method takes well defined color formats
	 * as arguments instead of specifying the depth and BPP manually.
	 */
	uint32_t frame_buffer_id;
	ret = drmModeAddFB(
		drm_fd,
		chosen_resolution->hdisplay, chosen_resolution->vdisplay,
		24, 32, create_request.pitch,
		create_request.handle, &frame_buffer_id
	);

	/* Without framebuffer, we won't do anything so bail out ! */
	if (ret) {
		LOG(
			"Could not add a framebuffer using drmModeAddFB : %s\n",
			strerror(ret)
		);
		goto could_not_add_frame_buffer;
	}

	/* We assume that the currently chosen encoder CRTC ID is the current
	 * one.
	 */
	uint32_t current_crtc_id = screen_encoder->crtc_id;

	if (!current_crtc_id) {
		LOG("The retrieved encoder has no CRTC attached... ?\n");
		goto could_not_retrieve_current_crtc;
	}

	/* Backup the informations of the CRTC to restore when we're done.
	 * The most important piece seems to currently be the buffer ID.
	 */
	drmModeCrtc * __restrict crtc_to_restore =
		drmModeGetCrtc(drm_fd, current_crtc_id);

	if (!crtc_to_restore) {
		LOG("Could not retrieve the current CRTC with a valid ID !\n");
		goto could_not_retrieve_current_crtc;
	}

	/* Set the CRTC so that it uses our new framebuffer */
	ret = drmModeSetCrtc(
		drm_fd, current_crtc_id, frame_buffer_id,
		0, 0,
		&valid_connector->connector_id,
		1,
		chosen_resolution);

	/* For this test only : Export our dumb buffer using PRIME */
	/* This will provide us a PRIME File Descriptor that we'll use to
	 * map the represented buffer. This could be also be used to reimport
	 * the GEM buffer into another GPU */
	struct drm_prime_handle prime_request = {
		.handle = create_request.handle,
		.flags  = DRM_CLOEXEC | DRM_RDWR,
		.fd     = -1
	};

	ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);
	int const dma_buf_fd = prime_request.fd;

	/* If we could not export the buffer, bail out since that's the
	 * purpose of our test */
	if (ret || dma_buf_fd < 0) {
		LOG(
			"Could not export buffer : %s (%d) - FD : %d\n",
			strerror(ret), ret,
			dma_buf_fd
 		);
		goto could_not_export_buffer;
	}

	/* Map the exported buffer, using the PRIME File descriptor */
	/* That ONLY works if the DRM driver implements gem_prime_mmap.
	 * This function is not implemented in most of the DRM drivers for 
	 * GPU with discrete memory. Meaning that it will surely fail with
	 * Radeon, AMDGPU and Nouveau drivers for desktop cards ! */
	uint8_t * primed_framebuffer = mmap(
		0, create_request.size,	PROT_READ | PROT_WRITE, MAP_SHARED,
		dma_buf_fd, 0);
	ret = errno;

	/* Bail out if we could not map the framebuffer using this method */
	if (primed_framebuffer == NULL || primed_framebuffer == MAP_FAILED) {
		LOG(
			"Could not map buffer exported through PRIME : %s (%d)\n"
			"Buffer : %p\n",
			strerror(ret), ret,
			primed_framebuffer
		);
		goto could_not_map_buffer;
	}

	LOG("Buffer mapped !\n");

	/* The fun begins ! At last !
	 * We'll do something simple :
	 * We'll list a row of pixel, on the screen, starting from the top,
	 * down to the bottom of screen, using either Red, Blue or Green
	 * randomly, each time we press Enter.
	 * If we press 'q' and then Enter, the process will stop.
	 * The process will also stop once we've reached the bottom of the
	 * screen.
	 */
	uint32_t const bytes_per_pixel = 4;
	uint_fast64_t
	  pixel = 0,
	  size = create_request.size;

	/* Cleanup the framebuffer */
	memset(primed_framebuffer, 0, size);

	/* The colors table */
	uint32_t const red   = (0xff<<16);
	uint32_t const green = (0xff<<8);
	uint32_t const blue  = (0xff);
	uint32_t const colors[] = {red, green, blue};

	/* Pitch is the stride in bytes.
	 * However, for our purpose we'd like to know the stride in pixels.
	 * So we'll divide the pitch (in bytes) by the number of bytes
	 * composing a pixel to get that information.
	 */
	uint32_t const stride_pixel = create_request.pitch / bytes_per_pixel;
	uint32_t const width_pixel  = create_request.width;

	/* The width is padded so that each row starts with a specific
	 * alignment. That means that we have useless pixels that we could
	 * avoid dealing with in the first place.
	 * Now, it might be faster to just lit these useless pixels and get
	 * done with it. */
	uint32_t const diff_between_width_and_stride =
		stride_pixel - width_pixel;
	uint_fast64_t const size_in_pixels =
		create_request.height * stride_pixel;

	/* While we didn't get a 'q' + Enter or reached the bottom of the
	 * screen... */
	while (getc(stdin) != 'q' && pixel < size_in_pixels) {
		/* Choose a random color. 3 being the size of the colors table. */
		uint32_t current_color = colors[rand()%3];

		/* Color every pixel of the row.
		 * Now, the framebuffer is linear. Meaning that the first pixel of
		 * the first row should be at index 0, but the first pixel of the
		 * second row should be at index (stride+0) and the first pixel of
		 * the n-th row should be at (n*stride+0).
		 * 
		 * Instead of computing the value, we'll just increment the "pixel"
		 * index and accumulate the padding once done with the current row,
		 * in order to be ready to start for the next row.
		 */
		for (uint_fast32_t p = 0; p < width_pixel; p++)
			((uint32_t *) primed_framebuffer)[pixel++] = current_color;
		pixel += diff_between_width_and_stride;
		//LOG("pixel : %lu, size : %lu\n", pixel, size_in_pixels);
	}

	munmap(primed_framebuffer, create_request.size);

could_not_map_buffer:
could_not_export_buffer:
	// very ugly but will do for an example...
	{
		struct drm_mode_destroy_dumb destroy_request = {
			.handle = create_request.handle
		};
		
		ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_request);
	}
	drmModeSetCrtc(
		drm_fd,
		crtc_to_restore->crtc_id, crtc_to_restore->buffer_id,
		0, 0, &valid_connector->connector_id, 1, &crtc_to_restore->mode
	);
could_not_retrieve_current_crtc:
	drmModeRmFB(drm_fd, frame_buffer_id);
could_not_add_frame_buffer:
could_not_allocate_buffer:
	drmModeFreeEncoder(screen_encoder);
could_not_retrieve_encoder:
	drmModeFreeModeInfo(chosen_resolution);
no_valid_resolution:
	drmModeFreeConnector(valid_connector);
no_valid_connector:
	close(drm_fd);
could_not_open_drm_fd:
	return 0;
}

