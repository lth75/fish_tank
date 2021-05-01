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

#define PORT 3501
const unsigned char UDP_HEADER[3]={'I','a','n'}; 
struct sockaddr_in local_addr,addr_from;
struct sockaddr_in pi_addr;
int pi_addr_len;
unsigned char g_buf[1024];
unsigned char g_cmd[1024];

struct tag_board_state {
	int relay_mask;
	int temperature;
	int wetness;
	bool is_connected;
} g_state;

void print_state(struct tag_board_state* state_ptr, const char *str)
{
	if(str)
		printf("%s",str);
	printf("\trelay mask:%x\n",state_ptr->relay_mask);
	printf("\ttemperature:%f\n",state_ptr->temperature/10.0);
	printf("\ttemperature:%f\n",state_ptr->wetness/10.0);
	printf("\tis_connected:%d\n",state_ptr->is_connected);
}

void print_udp(unsigned char *buf)
{
	printf("udp 0:");
	for(int i=0;i<16;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\nudp 1:");
	for(int i=16;i<32;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\n");
}

int bytes_to_int(unsigned char *buf)
{
	int ret=(buf[1]);
	ret<<=8;
	ret+=(buf[0]);
	return ret;
}

void int_to_bytes(unsigned char *buf, int value)
{
	buf[0]=(unsigned char)(value&0xff);
	buf[1]=(unsigned char)(value>>8);
}

int main()
{
	memset(&g_state,0,sizeof(g_state));
	int serverSocket,peerlen,buflen;
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
	
	peerlen=sizeof(addr_from);
	while(true)
	{
		buflen=recvfrom(serverSocket,(char*)g_buf,sizeof(g_buf),0,(struct sockaddr *)&addr_from,(socklen_t*)&peerlen);
#ifdef X86_SIM		
		print_udp(g_buf);
		if(buflen<0)
			perror("buflen negative:");
#endif	
		if(buflen<0) continue;

		if(memcmp(g_buf,UDP_HEADER,3)!=0)
			continue;
		if(g_buf[3]>=1)
		{  // data from raspberry
			memcpy(&pi_addr,&addr_from,peerlen);
			pi_addr_len=peerlen;
			// handle message
			switch(g_buf[4])
			{
			case 1: // data report
				switch(g_buf[5])
				{
				case 0: // initial connect, 
#ifdef X86_SIM
					printf("initial state request from rpi received\n");
#endif			
					g_state.relay_mask=bytes_to_int(g_buf+6);
					g_state.temperature=bytes_to_int(g_buf+8);
					g_state.wetness=bytes_to_int(g_buf+10);
					g_state.is_connected=true;
					break;
				case 2:
					g_state.relay_mask=bytes_to_int(g_buf+6);
					break;
				case 3:
					g_state.temperature=bytes_to_int(g_buf+6);
					g_state.wetness=bytes_to_int(g_buf+8);
					break;
				default:
					;// unknown command received
				}
				break;
			default:
				;
			}		
#ifdef X86_SIM				
			print_state(&g_state,"state after rpi msg\n");
#endif			
		}	
		else // data from web server
		{
			if(g_buf[4]!=1) continue; // unknown command
			// handle message
			switch(g_buf[5])
			{
			case 1: // write relay
				if(!g_state.is_connected)
					break;
				// update g_state and forward message to down stream
				{
					int mask=1<<g_buf[6];
					if(g_buf[7])
						g_state.relay_mask|=mask;
					else
						g_state.relay_mask&=~mask;
				}
				memcpy(g_cmd,g_buf,32);
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&pi_addr,pi_addr_len);
				break;
			case 2: // read relay
				memcpy(g_cmd,g_buf,32);
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=2;
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&pi_addr,pi_addr_len);
				// response to server
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=2;
				int_to_bytes(g_cmd+6,g_state.relay_mask);
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr *)&addr_from,peerlen);
				break;
			case 3: // read temperature and wetness
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=3;
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&pi_addr,pi_addr_len);
				// response to server
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=3;
				int_to_bytes(g_cmd+6,g_state.temperature);
				int_to_bytes(g_cmd+8,g_state.wetness);
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr *)&addr_from,peerlen);
				break;
			case 4: // fish food delivery
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=1;
				g_cmd[4]=1;
				g_cmd[5]=4;
				sendto(serverSocket,g_cmd,32,0,(struct sockaddr*)&pi_addr,pi_addr_len);
				break;
			default:
				; // unknown command
			}
		}	
	} 

	close(serverSocket);
	printf("%d",errno);
 	return 0;
}
