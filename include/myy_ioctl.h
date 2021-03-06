#ifndef NYANBADA_INCLUDE_MYY_IOCTL_H
#define NYANBADA_INCLUDE_MYY_IOCTL_H 1

// -- IOCTLS BEGIN

#include <linux/ioctl.h>
#include <linux/types.h>

// magic number actually unused by other drivers
#define MYY_IOCTL_BASE 0xB6

#define MYY_IO(nr)         _IO(MYY_IOCTL_BASE, nr)
#define MYY_IOR(nr, type) _IOR(MYY_IOCTL_BASE, nr, type)

#define MYY_IOCTL_HELLO                MYY_IO(0x1)
#define MYY_IOCTL_TEST_FD_PASSING     MYY_IOR(0x2, __s32)
#define MYY_IOCTL_TEST_DMA_FD_PASSING MYY_IOR(0x3, __s32)

// -- IOCTLS END

#endif
