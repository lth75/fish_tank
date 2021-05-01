#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h>
#include <signal.h>
#include <termios.h> 
#include <unistd.h>
#include <fcntl.h> 

#define PORT_CMD 3503
#define UART_HEADER 0xf5
struct sockaddr_in local_addr,addr_from;
unsigned char g_buf[1024];
unsigned char g_cmd[1024];

int sock_down, g_uart;

void clean_up()
{
 	close(g_uart);
  	close(sock_down);
}

void my_handler(int s)
{
	clean_up();
	exit(1); 
}

int serialOpen(const char *dev, int baud)
{
	int fd=open(dev,O_RDWR|O_NOCTTY|O_NDELAY);
	if(fd==-1)
	{
		printf("unable to open tty:%s\n",dev);
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

int main()
{
    struct timeval wait_time;
	struct sigaction sigIntHandler;
	int ret;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);
   
	int peerlen,buflen;
	if(-1==(sock_down=socket(AF_INET,SOCK_DGRAM,0)))
	  puts("create socket failed\n");
	else
	  puts("create socket succeeded\n");
	 
	bzero(&local_addr, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(PORT_CMD);
	local_addr.sin_addr.s_addr =INADDR_ANY;
	if(bind(sock_down,(struct sockaddr*)&local_addr,
	sizeof(local_addr))==-1)
	{
	  //printf("%s\n","bind failed");
	  perror("bind failed:");
	  return -1;
	}
	else 
	  printf("%s\n","bind succeeded");
  	
	g_uart=serialOpen("/dev/ttyS11",9600);
	
	fd_set read_fds;
	while(true)
	{
		FD_ZERO(&read_fds);
        FD_SET(g_uart,&read_fds);
        FD_SET(sock_down,&read_fds);
        
        wait_time.tv_sec=5;
	 	wait_time.tv_usec=0;
        ret = select(g_uart+1,&read_fds,NULL,NULL,&wait_time);
        if(ret < 0)
        {
            printf("Fail to select!\n");
            return -1;
        }
        if(ret==0)
        	continue;
        if(FD_ISSET(g_uart, &read_fds))
        {
            ret = read(g_uart,g_buf,16);
            if(ret > 0)
            {
            	printf("uart cmd received 0:");
            	for(int i=0;i<16;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\n");
            }
            else
            	perror("uart read failed:");
        }
        else if(FD_ISSET(sock_down,&read_fds))
        {
        	ret = recv(sock_down,g_buf,16,0);
        	if(ret>0)
        	{
        		printf("trigger cmd received 0:");
            	for(int i=0;i<16;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\n");
        		// forward to sock up
        		write(g_uart,g_buf,16);
        	}
        	else
        		perror("receive data from sock down failed:");
        }
	}
	clean_up();
 	return 0;
}
	
