#ifndef OLED_IOCTL_H
#define OLED_IOCTL_H
#include <linux/ioctl.h>
#define IOCTL_MAGIC 'E'
#define CLEAN_SEQ_NO 0x01
#define INV_SEQ_NO 0x02
#define INS_SEQ_NO 0x03
#define CON_SEQ_NO 0x04


#define IO_CLEAN _IO(IOCTL_MAGIC,CLEAN_SEQ_NO)
#define IO_INV _IO(IOCTL_MAGIC,INV_SEQ_NO)
#define IO_INS _IO(IOCTL_MAGIC,INS_SEQ_NO)

#define IO_CON _IOW(IOCTL_MAGIC,CON_SEQ_NO,int)
#endif
