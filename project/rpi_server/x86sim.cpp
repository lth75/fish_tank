#include <x86sim.h>
#include <stdio.h>
#include <termios.h> 
#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h>
#define UART_NAME "/dev/ttyS10"
int serialOpen(const char *dev, int baud)
{
	int fd=open(UART_NAME,O_RDWR|O_NOCTTY|O_NDELAY);
	if(fd==-1)
	{
		printf("unable to open tty:%s\n",UART_NAME);
		return -1;
	}
	if(isatty(fd)==0)
		printf("%s is not a tty device\n",dev);
	struct  termios newtio;
	bzero(&newtio,sizeof(newtio));
	/*8N1*/
	newtio.c_cflag = B9600|CS8|CLOCAL|CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/*正常模式*/
	/*newtio.c_lflag = ICANON;*/

	/*非正常模式*/
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 10;

	tcflush(fd,TCIFLUSH);
	/*新的temios作为通讯端口参数*/
	tcsetattr(fd,TCSANOW,&newtio);
	return fd;
}
 
int wiringPiSetup()
{
	return 0;
}
