#ifndef OLED_IOCTL_H
#define OLED_IOCTL_H
#include <linux/ioctl.h>
#define IOCTL_MAGIC 'E'
#define CLEAN_SEQ_NO 0x01

#define IO_CLEAN _IO(IOCTL_MAGIC,CLEAN_SEQ_NO)

#endif
