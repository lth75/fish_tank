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
 
int main(int argc, char*argv[])
{
	int content_len;
    int   x;
    int   i;
    char   *p;
    char   *pRequestMethod;     /*   METHOD属性值   */
    setvbuf(stdin,NULL,_IONBF,0);     /*关闭stdin的缓冲*/
    printf("Content-type:application/json\r\n");     /*从stdout中输出，告诉Web服务器返回的信息类型*/
    printf("\r\n");                                           /*插入一个空行，结束头部信息*/
    pRequestMethod = getenv("REQUEST_METHOD");
     if (strcasecmp(pRequestMethod,"POST")==0)
    {
        // printf("<p>OK the method is POST!\n</p>");
        p = getenv("CONTENT_LENGTH");     //从环境变量CONTENT_LENGTH中得到数据长度    
        if (p!=NULL)
        {
        	content_len = atoi(p);
        }
        else
        {
        	content_len = 0;
        }
        if (content_len > sizeof(g_buf)-1)   {
                content_len = sizeof (g_buf) - 1;
        }

		content_len=fread(g_buf,content_len,1,stdin);
		if(content_len>0)
		{
			cJSON *root=cJSON_Parse(g_buf);
			cJSON *cmd=cJSON_GetObjectItem(root,"cmd");
			cJSON *cmd_data;
			int ret_len;
			switch(cmd->valueint)
			{
			case 1:// set relay state
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=0;
				g_cmd[4]=1;
				g_cmd[5]=1;
				cmd_data=cJSON_GetObjectItem(root,"data");
				g_cmd[6]=(unsigned char)cmd_data[0];
				g_cmd[7]=(unsigned char)cmd_data[1];
				send_udp(g_cmd,16,false);
				printf("{\"ret_code\"=0}\n");
				break;
			case 2:
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=0;
				g_cmd[4]=1;
				g_cmd[5]=2;
				ret_len=send_udp(g_cmd,32,true);
				if(ret_len>=32)
				{
					int relay_mask=g_cmd[6]+g_cmd[7]*256;
					printf("{");
					printf("\"ret_code\"=0,");
					printf("\"cmd\"=2,");
					printf("\"data\"=[");
					for(int i=0;i<7;i++)
						printf("%d,",(relay_mask&(1<<i)));
					printf("%d]",(relay_mask&(1<<7)));
					printf("}\n");
				}
				else
				{
					printf("{\"ret_code\"=-1}\n");
				}
				break;
			case 3:
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=0;
				g_cmd[4]=1;
				g_cmd[5]=3;
				ret_len=send_udp(g_cmd,32,true);
				if(ret_len>=32)
				{
					int temperature=g_cmd[6]+g_cmd[7]*256;
					int wetness=g_cmd[8]+g_cmd[9]*256;
					printf("{");
					printf("\"ret_code\"=0,");
					printf("\"cmd\"=3,");
					printf("\"data\"=[%d,%d]",temperature,wetness);
					printf("}\n");
				}
				else
				{
					printf("{\"ret_code\"=-1}\n");
				}
				break;
			case 4: // fish food delivery
				memcpy(g_cmd,UDP_HEADER,3);
				g_cmd[3]=0;
				g_cmd[4]=1;
				g_cmd[5]=4;
				ret_len=send_udp(g_cmd,32,false);
				printf("{\"ret_code\"=0}\n");
			default:
				; // unknown cmd
			}
		}
    }
    return   0;
}

#define PORT	3501
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
 
	if(-1==(sendto(clientSocket,buf,len,0, (struct sockaddr *)&remoteaddr,addrlen=sizeof(remoteaddr))))
    {
    	close(clientSocket);
    	return -1;
    }
	
	if(waitResponse)
	{
		buflen=recvfrom(clientSocket,buf,sizeof(buf),0,(struct sockaddr *)&remoteaddr,&addrlen);
		close(clientSocket);
		return buflen;
	}
	close(clientSocket);
	return 0;
}
