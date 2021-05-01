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
struct sockaddr_in g_server_addr;
int server_addr_len;
unsigned char g_buf[1024];
unsigned char g_cmd[1024];

int sock_up,sock_down;

void clean_up()
{
 	close(sock_up);
  	close(sock_down);
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
	if(-1==(sock_up=socket(AF_INET,SOCK_DGRAM,0)))
  		// puts("socket开启失败");
  		return -1;  
  	
  	bzero(&g_server_addr, sizeof(g_server_addr));
	g_server_addr.sin_family = AF_INET;
	g_server_addr.sin_port = htons(PORT);
	g_server_addr.sin_addr.s_addr =inet_addr("127.0.0.1");
	server_addr_len=sizeof(g_server_addr);
	
	fd_set read_fds;
	while(true)
	{
		FD_ZERO(&read_fds);
        FD_SET(sock_up,&read_fds);
        FD_SET(sock_down,&read_fds);
        
        wait_time.tv_sec=5;
	 	wait_time.tv_usec=0;
        ret = select(sock_up+1,&read_fds,NULL,NULL,&wait_time);
        if(ret < 0)
        {
            printf("Fail to select!\n");
            return -1;
        }
        if(ret==0)
        	continue;
        if(FD_ISSET(sock_up, &read_fds))
        {
            ret = recv(sock_up,g_buf,32,0);
            if(ret > 0)
            {
            	printf("server cmd received 0:");
            	for(int i=0;i<16;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\nserver cmd received 1:");
            	for(int i=16;i<32;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\n");
            }
            else
            	perror("receive from sock up failed:");
        }
        else if(FD_ISSET(sock_down,&read_fds))
        {
        	ret = recv(sock_down,g_buf,32,0);
        	if(ret>0)
        	{
        		printf("trigger cmd received 0:");
            	for(int i=0;i<16;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\ntrigger cmd received 1:");
            	for(int i=16;i<32;i++)
            		printf("%02x ", g_buf[i]);
            	printf("\n");
        		// forward to sock up
        		sendto(sock_up,g_buf,32,0, (struct sockaddr *)&g_server_addr,server_addr_len);
        	}
        	else
        		perror("receive data from sock down failed:");
        }
	}
	clean_up();
 	return 0;
}
	
