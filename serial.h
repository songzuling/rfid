#ifndef SERIAL_H_
#define SERIAL_H_

#include"stdio.h"
#include"stdlib.h"
#include"unistd.h"
#include"sys/types.h"
#include"fcntl.h"
#include"sys/stat.h"
#include"termios.h"
#include"errno.h"

extern unsigned long int RxTotalBytes;
extern unsigned long int RxTotalFrameCnt;
extern unsigned long int RxCorrectFrameCnt;
extern unsigned long int RxErrorFrameCnt;

extern int serialport_init(char *port,int baud);
extern int open_port(char *port);
extern int set_port(int fd, int baudrate);

#endif
