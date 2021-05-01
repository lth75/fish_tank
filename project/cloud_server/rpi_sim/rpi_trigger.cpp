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
#include <cJSON.h>
const unsigned char UDP_HEADER[3]={'I','a','n'}; 
char g_buf[1024];
char g_udp[256];
int send_udp(char *buf, int len, bool waitResponse);

void print_cmd(char *buf)
{
	printf("cmd:");
	for(int i=0;i<16;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\ncmd:");
	for(int i=16;i<32;i++)
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

void int_to_bytes(char *buf, int value)
{
	buf[0]=(char)(value&0xff);
	buf[1]=(char)(value>>8);
}
 
int main(int argc, char*argv[])
{
	int content_len;
    int   x;
    int   i;
    char   *p;

    setvbuf(stdin,NULL,_IONBF,0);     /*关闭stdin的缓冲*/
	content_len=fread(g_buf,1,1024,stdin);
	printf("read length:%d\n",content_len);
	if(content_len>0)
	{
		printf("read json data\n");
		cJSON *root=cJSON_Parse(g_buf);
		cJSON *cmd=cJSON_GetObjectItem(root,"cmd");
		cJSON *addr=cJSON_GetObjectItem(root,"addr");
		cJSON *cmd_data;
		cmd_data=cJSON_GetObjectItem(root,"data");
		int ret_len;
		switch(cmd->valueint)
		{
		case 0: // init state
			memcpy(g_udp,UDP_HEADER,3);
			g_udp[3]=(char)(addr->valueint);
			g_udp[4]=1;
			g_udp[5]=0;
			int_to_bytes(g_udp+6,cJSON_GetArrayItem(cmd_data,0)->valueint);
			int_to_bytes(g_udp+8,cJSON_GetArrayItem(cmd_data,1)->valueint);
			int_to_bytes(g_udp+10,cJSON_GetArrayItem(cmd_data,2)->valueint);
			ret_len=send_udp(g_udp,32,false);
			break;
		case 1:// set relay state
			memcpy(g_udp,UDP_HEADER,3);
			g_udp[3]=(char)(addr->valueint);
			g_udp[4]=1;
			g_udp[5]=1;			
			g_udp[6]=(unsigned char)(cJSON_GetArrayItem(cmd_data,0)->valueint);
			g_udp[7]=(unsigned char)(cJSON_GetArrayItem(cmd_data,1)->valueint);
			send_udp(g_udp,16,false);
			break;
		case 2:
			memcpy(g_udp,UDP_HEADER,3);
			g_udp[3]=(char)(addr->valueint);
			g_udp[4]=1;
			g_udp[5]=2;
			int_to_bytes(g_udp+6,cJSON_GetArrayItem(cmd_data,0)->valueint);
			ret_len=send_udp(g_udp,32,false);
			break;
		case 3:
			memcpy(g_udp,UDP_HEADER,3);
			g_udp[3]=(char)(addr->valueint);
			g_udp[4]=1;
			g_udp[5]=3;
			int_to_bytes(g_udp+6,cJSON_GetArrayItem(cmd_data,0)->valueint);
			int_to_bytes(g_udp+8,cJSON_GetArrayItem(cmd_data,1)->valueint);
			ret_len=send_udp(g_udp,32,false);
			break;
		case 4: // fish food delivery
			memcpy(g_udp,UDP_HEADER,3);
			g_udp[3]=(char)(addr->valueint);
			g_udp[4]=1;
			g_udp[5]=4;
			ret_len=send_udp(g_udp,32,false);
			break;
		default:
			; // unknown cmd
		}
	}
    return   0;
}

#define PORT	3502
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
