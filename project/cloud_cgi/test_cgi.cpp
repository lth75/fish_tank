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
const unsigned char UDP_HEADER[3]={'I','a','n'}; 
int serverSocket=-1;
struct sockaddr_in local_addr;
struct sockaddr_storage addr_from;
struct sockaddr_in pi_addr;
int pi_addr_len;
socklen_t peerlen;

unsigned char g_buf[1024];
unsigned char g_cmd[1024];
struct tag_board_state {
	int relay_mask;
	int temperature;
	int wetness;
	bool is_connected;
} g_state;


void my_handler(int s)
{
	if(serverSocket>-1)
	{
		close(serverSocket);		
	}
	exit(1); 
}

 
void print_cmd(unsigned char *buf)
{
	printf("cmd:");
	for(int i=0;i<16;i++)
		printf("%02x ",buf[i]);
	printf("\ncmd:");
	for(int i=16;i<32;i++)
		printf("%02x ",buf[i]);
	printf("\n");
}

int main()
{

   struct sigaction sigIntHandler;
 
   sigIntHandler.sa_handler = my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
 
   sigaction(SIGINT, &sigIntHandler, NULL);

	memset(&g_state,0,sizeof(g_state));
	int buflen;
	if(-1==(serverSocket=socket(AF_INET,SOCK_DGRAM,0)))
	  puts("create socket failed\n");
	else
	  puts("create socket succeeded\n");
	 
	bzero(&local_addr, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(PORT);
	local_addr.sin_addr.s_addr =INADDR_ANY;
	if(bind(serverSocket,(struct sockaddr*)&local_addr,
	sizeof(local_addr))==-1)
	  printf("%s\n","bind failed");
	else 
	  printf("%s\n","bind succeeded");
	
	while(true)
	{
		peerlen=sizeof(struct sockaddr_storage);
		buflen=recvfrom(serverSocket,(char*)g_buf,64,0,(struct sockaddr *)&addr_from,&peerlen);
		if(buflen<0) 
		{
			perror("packet received with error:");
			printf("\n");
		}
		printf("packet received:%d\n",buflen);
		print_cmd(g_buf);		
		if(buflen<0) continue;
		if(memcmp(g_buf,UDP_HEADER,3)!=0)
		{
			printf("header not match\n");
			continue;
		}
		if(g_buf[3]>=1)
		{  
			printf("invalid address\n");
			return 0;		
		}	
		else // data from web server
		{
			if(g_buf[4]!=1) continue; // unknown command
			// handle message
			switch(g_buf[5])
			{
			case 1: // write relay
				printf("relay set command received:%d,%d\n",g_buf[6],g_buf[7]);
				break;
			case 2: // read relay
				printf("read relay cmd received\n");
				printf("response mask:0xf531\n");
				g_state.relay_mask=0xf531;
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=2;
				g_cmd[6]=(unsigned char)(0xff&g_state.relay_mask);
				g_cmd[7]=(unsigned char)(0xff&(g_state.relay_mask>>8));
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&addr_from,peerlen);
				break;
			case 3: // read temperature and wetness
				printf("response temperature 21.3, wetness tds 2.1\n");
				g_state.temperature=213;
				g_state.wetness=21;
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=3;
				g_cmd[6]=g_state.temperature&0xff;
				g_cmd[7]=(g_state.temperature>>8)&0xff;
				g_cmd[8]=g_state.wetness&0xff;
				g_cmd[9]=(g_state.temperature>>8)&0xff;
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&addr_from,peerlen);
				break;
			case 4: // fish food delivery
				printf("fish food delivery cmd received\n");
				break;
			default:
				; // unknown command
			}
			break;
		}	
	} 

	close(serverSocket);
	printf("%d",errno);
 	return 0;
}
