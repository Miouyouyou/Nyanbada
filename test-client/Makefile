LDFLAGS = `pkg-config libdrm --libs --cflags`
BIN_DIR = bin

all:
	$(CC) -o ${BIN_DIR}/test-ioctl test-ioctl.c ${LDFLAGS}
	$(CC) -o ${BIN_DIR}/test-drm-itself test-drm-itself.c common-drm-functions.c ${LDFLAGS}
	$(CC) -o ${BIN_DIR}/test-prime-dumb test-prime-dumb.c ${LDFLAGS}
	$(CC) -o ${BIN_DIR}/test-prime-dumb-kms test-prime-dumb-kms.c ${LDFLAGS}
	$(CC) -o ${BIN_DIR}/test-prime-radeon test-prime-radeon.c ${LDFLAGS}
	$(CC) -o ${BIN_DIR}/test-drm-radeon test-drm-radeon.c ${LDFLAGS} -ldrm_amdgpu

clean:
	$(RM) ${BIN_DIR}/*
