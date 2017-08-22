all:
	$(CC) -o test-drm-itself test-drm-itself.c common-drm-functions.c `pkg-config libdrm --libs --cflags`

clean:
	$(RM) client
