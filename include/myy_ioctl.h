#ifndef NYANBADA_INCLUDE_MYY_IOCTL_H
#define NYANBADA_INCLUDE_MYY_IOCTL_H 1

// -- IOCTLS BEGIN

// magic number actually unused by other drivers
#define MYY_IOCTL_BASE 0xB6
#define MYY_IO(nr) _IO(MYY_IOCTL_BASE, nr)

#define MYY_IOCTL_HELLO MYY_IO(0x1)

// -- IOCTLS END

#endif
