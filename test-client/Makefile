all:
	$(CC) -o client client.c common-drm-functions.c `pkg-config libdrm --libs --cflags`

clean:
	$(RM) client
