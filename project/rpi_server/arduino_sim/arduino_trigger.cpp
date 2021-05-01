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

char g_buf[1024];
char g_udp[1024];
int send_udp(char *buf, int len, bool waitResponse);

void print_cmd(char *buf)
{
	printf("cmd:");
	for(int i=0;i<16;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\n");
}

int bytes_to_int(char *buf)
{
	int ret=(unsigned char)(buf[1]);
	ret<<=8;
	ret+=(unsigned char)(buf[0]);
	return ret;
}
 
void fill_uart_cmd_check(char *data)
{
	char c=data[0];
	for(int i=1;i<15;i++)
	{
		c^=data[i];
	}
	data[15]=c;
}
 
int main(int argc, char*argv[])
{
	int content_len=fread(g_udp,1024,1,stdin);
	int t;
	for(int i=0;i<16;i++)
	{
		sscanf(g_udp+i*3,"%x",&t);
		g_buf[i]=(char)t;
	}
	fill_uart_cmd_check(g_buf);
	print_cmd(g_buf);
	send_udp(g_buf,16,false);
    return   0;
}

#define PORT	3503
int send_udp(char *buf, int len, bool waitResponse)
{
	int clientSocket,buflen,addrlen,peerlen;
	char addr[]="127.0.0.1";
	struct sockaddr_in remoteaddr,addr_from;
 
	if(-1==(clientSocket=socket(AF_INET,SOCK_DGRAM,0)))
  		// puts("socket开启失败");
  		return -1;
 
	bzero(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(PORT);
	remoteaddr.sin_addr.s_addr =inet_addr(addr);
#ifdef X86_SIM
 	printf("send packet\n");
#endif 	
	if(-1==(sendto(clientSocket,buf,len,0, (struct sockaddr *)&remoteaddr,addrlen=sizeof(remoteaddr))))
    {
    	close(clientSocket);
    	return -1;
    }
	
	if(waitResponse)
	{
#ifdef X86_SIM
		printf("packet sent\n");
		printf("wait for response\n");
#endif	
		buflen=recvfrom(clientSocket,buf,32,0,(struct sockaddr *)&remoteaddr,(socklen_t*)&addrlen);
#ifdef X86_SIM
		print_cmd(buf);
#endif		
		close(clientSocket);
		return buflen;
	}
	close(clientSocket);
	return 0;
}
