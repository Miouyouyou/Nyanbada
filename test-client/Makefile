LDFLAGS = `pkg-config libdrm --libs --cflags`

all:
	$(CC) -o test-drm-itself test-drm-itself.c common-drm-functions.c ${LDFLAGS}
	$(CC) -o test-prime-dumb test-prime-dumb.c ${LDFLAGS}
	$(CC) -o test-prime-dumb-kms test-prime-dumb-kms.c ${LDFLAGS}
	$(CC) -o test-prime-radeon test-prime-radeon.c ${LDFLAGS}
	$(CC) -o test-drm-radeon test-drm-radeon.c ${LDFLAGS} -ldrm_amdgpu
	

clean:
	$(RM) client
