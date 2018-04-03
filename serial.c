#include"serial.h"

int serialport_init(char *port, int baud)
{
    int fd = open_port(port);
    set_port(fd,baud);
    return fd;
}

// open port
int open_port(char *port)
{    
    char portName[15];
    sprintf(portName,"/dev/tty%4s",port);
    int fd = open((const char*)portName, O_RDWR|O_NONBLOCK);
    if(fd == -1)
    {
        perror("Can not open serial port!\r\n");
    }
    return fd;
}


// set port parameter
int set_port(int fd, int baudrate)
{
    struct termios newtio;

    newtio.c_cflag |= CLOCAL|CREAD;
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;          // 8
    newtio.c_cflag &= ~PARENB;      // N
    newtio.c_cflag &= ~CSTOPB;      // 1

    newtio.c_iflag &= ~ISTRIP;      // !!!!!!!!!!!!!!!!

    cfsetispeed(&newtio, baudrate);  // bauderate
    cfsetospeed(&newtio, baudrate);

    tcflush(fd, TCIFLUSH);          // 处理未接收字符

    if( (tcsetattr(fd, TCSANOW, &newtio)) != 0 )
    {
        perror("COM set error\r\n!");
        return -1;
    }

    printf("COM open succeed!\r\n");
    return 0;
}
