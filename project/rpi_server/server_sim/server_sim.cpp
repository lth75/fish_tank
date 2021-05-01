#include<stdio.h>
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

#define PORT 3501
#define PORT_CMD 3502
const unsigned char UDP_HEADER[3]={'I','a','n'}; 
struct sockaddr_in local_addr,addr_from;
struct sockaddr_in rpi_addr;
int rpi_addr_len;
unsigned char g_buf[1024];
unsigned char g_cmd[1024];

int sock_rpi;

void clean_up()
{
  	close(sock_rpi);
}

void my_handler(int s)
{
	clean_up();
	exit(1); 
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
	if(-1==(sock_rpi=socket(AF_INET,SOCK_DGRAM,0)))
	  puts("create socket failed\n");
	else
	  puts("create socket succeeded\n");
	 
	bzero(&local_addr, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(PORT);
	local_addr.sin_addr.s_addr =INADDR_ANY;
	if(bind(sock_rpi,(struct sockaddr*)&local_addr,	sizeof(local_addr))==-1)
	{
	  //printf("%s\n","bind failed");
	  perror("bind failed:");
	  return -1;
	}
	else 
	  printf("%s\n","bind succeeded");

	fd_set read_fds;
	while(true)
	{
		FD_ZERO(&read_fds);
        FD_SET(sock_rpi,&read_fds);
        
        wait_time.tv_sec=5;
	 	wait_time.tv_usec=0;
        ret = select(sock_rpi+1,&read_fds,NULL,NULL,&wait_time);
        if(ret < 0)
        {
            printf("Fail to select!\n");
            return -1;
        }
        if(ret==0)
        	continue;
        peerlen=sizeof(addr_from);
        if(FD_ISSET(sock_rpi,&read_fds))
        {
        	ret = recvfrom(sock_rpi,g_buf,32,0,(struct sockaddr *)&addr_from,(socklen_t*)&peerlen);
        	if(ret>0)
        	{
        		if(g_buf[3]==1) // from rpi
        		{
        			memcpy(&rpi_addr,&addr_from,peerlen);
        			unsigned int addr=(unsigned int)(addr_from.sin_addr.s_addr);
        			unsigned int port1=htons(addr_from.sin_port);
        			printf("rpi addr:%d.%d.%d.%d, port:%d\n",(addr>>24),(addr>>16)&0xff, (addr>>8)&0xff, addr&0xff, port1);
        			printf("rpi addr:%x, port:%d\n",rpi_addr.sin_addr.s_addr, rpi_addr.sin_port);
        			rpi_addr_len=peerlen;
		        	printf("server cmd received 0:");
		        	for(int i=0;i<16;i++)
		        		printf("%02x ", g_buf[i]);
		        	printf("\nserver cmd received 1:");
		        	for(int i=16;i<32;i++)
		        		printf("%02x ", g_buf[i]);
		        	printf("\n");
        		}
        		else
        		{
		    		printf("trigger cmd received 0:");
		        	for(int i=0;i<16;i++)
		        		printf("%02x ", g_buf[i]);
		        	printf("\ntrigger cmd received 1:");
		        	for(int i=16;i<32;i++)
		        		printf("%02x ", g_buf[i]);
		        	printf("\n");
		    		// forward to rpi
		    		printf("send to:addr:%x, port:%d\n",rpi_addr.sin_addr.s_addr, rpi_addr.sin_port);
		    		g_buf[3]=1;// set to rpi addr
		    		ret = sendto(sock_rpi,g_buf,32,0, (struct sockaddr *)&rpi_addr,rpi_addr_len);
		    		if(ret<=0)
		    			perror("send packet failed:");
		    	}
        	}
        	else
        		perror("receive data from sock down failed:");
        }
	}
	clean_up();
 	return 0;
}
	
