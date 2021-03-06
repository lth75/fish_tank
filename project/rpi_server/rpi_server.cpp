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
#ifndef X86_SIM
#include <wiringPi.h>
#include <wiringSerial.h>
#else
#include <x86sim.h>
#endif
#define PORT 3501
#ifdef X86_SIM
#define SERVER_IP   "127.0.0.1"
#else
#define SERVER_IP	"47.117.1.147"
#endif
#define UART_DEV	"/dev/ttyS0"
#define UART_BAUD	9600
#define UART_HEADER 0xf5

const unsigned char UDP_HEADER[3]={'I','a','n'};
char g_temp[256]; 
char g_buf[1024];
int g_buf_head,g_buf_tail;
char g_udp[256];
int g_sock;
struct sockaddr_in g_server_addr;
int g_server_addr_len;
int g_uart;
struct tag_board_state {
	int relay_mask;
	int temperature;
	int wetness;
	bool is_connected;
} g_state;

void process_udp_cmd(char *buf, int len);
int send_udp(char *buf, int len, bool waitResponse);

void print_uart_packet(char *buf)
{
	printf("uart pt:");
	for(int i=0;i<16;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\n");
}
void print_udp_packet(char *buf)
{
	printf("udp 0:");
	for(int i=0;i<16;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\n");
	printf("udp 1:");
	for(int i=16;i<32;i++)
		printf("%02x ",(unsigned char)buf[i]);
	printf("\n");
}

void print_state(struct tag_board_state* state_ptr, const char *str)
{
	if(str)
		printf("%s",str);
	printf("\trelay mask:%x\n",state_ptr->relay_mask);
	printf("\ttemperature:%f\n",state_ptr->temperature/10.0);
	printf("\ttemperature:%f\n",state_ptr->wetness/10.0);
	printf("\tis_connected:%d\n",state_ptr->is_connected);
}

void init_uart_cmd(char *data)
{
	memset(data,0,16);
	data[0]=UART_HEADER;
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
void int_to_byte(int data, char *buf)
{
	buf[0]=(char)(data&0xff);
	buf[1]=(char)((data>>8)&0xff);
}

int byte_to_int(char *buf)
{
	int t;
	t=(unsigned char)buf[1];
	t<<=8;
	t+=(unsigned char)buf[0];
	return t;
}

bool check_uart_cmd(char *data)
{
	char c=data[0];
	for(int i=1;i<16;i++)
		c^=data[i];
#ifdef X86_SIM		
	if(c!=0)
		printf("check failed:%02x\n",(unsigned char)c);
#endif	
	return (c==0);
}
int main(int argc, char*argv[])
{
	struct timeval wait_time;
	int buflen,addrlen,peerlen;
	char addr[]="127.0.0.1";
	struct sockaddr_in addr_from;
	fd_set read_fds;
	int ret;
	memset(&g_state,0,sizeof(g_state));
 	if(wiringPiSetup() < 0)
 		return -1;
	if(-1==(g_sock=socket(AF_INET,SOCK_DGRAM,0)))
	{
  		perror("create socket failed:");
  		return -1;
 	}
	bzero(&g_server_addr, sizeof(g_server_addr));
	g_server_addr.sin_family = AF_INET;
	g_server_addr.sin_port = htons(PORT);
	g_server_addr.sin_addr.s_addr =inet_addr(SERVER_IP);
	g_server_addr_len=sizeof(g_server_addr);
 
	// open serial port
	g_uart=serialOpen(UART_DEV,9600);
	if(g_uart<0)
		return -1;
	g_buf_head=0;
	g_buf_tail=0;
	while(1)
	{
		// send cmd
		init_uart_cmd(g_temp);
		g_temp[1]=2;
		fill_uart_cmd_check(g_temp);
		write(g_uart,g_temp,16);
	 	// read status from adrduino
	 	FD_ZERO(&read_fds);
	 	FD_SET(g_uart,&read_fds);
	 	wait_time.tv_sec=5;
	 	wait_time.tv_usec=0;
	 	ret=select(g_uart+1,&read_fds,NULL,NULL,&wait_time);
	 	if(ret<0)
	 	{
	 		sleep(1000);
	 		continue;
	 	}
	 	bool bQuit=false;
	 	if(FD_ISSET(g_uart,&read_fds))
	 	{
	 		ret=read(g_uart,g_temp,16);		 		
	 		for(int i=0;i<ret;i++)
	 		{
	 			g_buf[g_buf_head]=g_temp[i];
	 			g_buf_head=(g_buf_head+1)&1023;
	 		}
#ifdef X86_SIM
			print_uart_packet(g_temp);
			printf("packet len:%d\n",ret);
			printf("head:%d, tail:%d, dist:%d\n",g_buf_head,g_buf_tail,(g_buf_head-g_buf_tail)&1023);
#endif		 		
	 		while( ((g_buf_head-g_buf_tail)&1023) >= 16)
	 		{
#ifdef X86_SIM
				printf("in loop\n");
#endif	 		
	 			if(g_buf[g_buf_tail]!=(char)UART_HEADER)
	 			{
#ifdef X86_SIM
					printf("invalid header:%02x, tail:%d\n",(unsigned char)g_buf[g_buf_tail],g_buf_tail);
#endif	 	 		
	 				g_buf_tail=(g_buf_tail+1)&1023;
	 				continue;
	 			}
#ifdef X86_SIM
				printf("head valid, handle uart cmd\n");
#endif	 			
	 			for(int i=0;i<16;i++)
	 				g_temp[i]=g_buf[(g_buf_tail+i)&1023];
	 			if(!check_uart_cmd(g_temp))
	 				continue;
	 			if(g_temp[1]==2)
		 		{// ready to process feed back
		 			g_state.relay_mask=byte_to_int(g_temp+2);
		 			g_buf_tail=(g_buf_tail+16)&1023;
		 			bQuit=true;
		 			break;
		 		}
		 		else
		 			g_buf_tail=(g_buf_tail+16)&1023;
	 		}
	 		if(bQuit)
	 			break;
	 	}
	}
	
	int loop_state=0;
	bool is_init_sent=false;
#ifdef X86_SIM	
	printf("enter main loop\n");
#endif
	while(1)
    {
        FD_ZERO(&read_fds);
        FD_SET(g_sock,&read_fds);
        FD_SET(g_uart,&read_fds);
        //  FD_SET(g_sock,&exception_fds);
        wait_time.tv_sec=5;
	 	wait_time.tv_usec=0;
        ret = select((g_uart>g_sock)? g_uart+1:g_sock+1,&read_fds,NULL,NULL,&wait_time);
        if(ret < 0)
        {
            printf("Fail to select!\n");
            return -1;
        }
        
        if(ret==0)
        {
        	loop_state=(loop_state+1)%600;
        	if(loop_state&1)
        	{
        		init_uart_cmd(g_temp);
        		g_temp[1]=2;
        		fill_uart_cmd_check(g_temp);
        		write(g_uart,g_temp,16);
        	}
        	if(loop_state%3==1)
        	{
        		init_uart_cmd(g_temp);
        		g_temp[1]=3;
        		fill_uart_cmd_check(g_temp);
        		write(g_uart,g_temp,16);
        	}
        	if(loop_state%20==0)
        	//if(loop_state%5==0)
        	{
        		memcpy(g_udp,UDP_HEADER,3);
			 	g_udp[3]=1;
			 	g_udp[4]=0;
			 	int_to_byte(g_state.relay_mask,g_udp+5);
				int_to_byte(g_state.temperature,g_udp+7);
				int_to_byte(g_state.wetness,g_udp+9);
				send_udp(g_udp,32,false);
        	}
        	continue;
        }
                
        if(FD_ISSET(g_sock, &read_fds))
        {
            ret = recv(g_sock,g_udp,32,0);
#ifdef X86_SIM
			print_udp_packet(g_udp);
			printf("packet len:%d\n",ret);
#endif	             
            if(ret > 0)
            {
            	if(memcmp(g_udp,UDP_HEADER,3)!=0)
            		continue;
            	// process the command
            	process_udp_cmd(g_udp,32);
            }         
        }
        else if(FD_ISSET(g_uart,&read_fds))
        {
        	ret=read(g_uart,g_temp,16);
#ifdef X86_SIM
			print_uart_packet(g_temp);
			printf("packet len:%d\n",ret);
			printf("head:%d, tail:%d, dist:%d\n",g_buf_head,g_buf_tail,(g_buf_head-g_buf_tail)&1023);
#endif	             
        	
        	for(int i=0;i<ret;i++)
	 		{
	 			g_buf[g_buf_head]=g_temp[i];
	 			g_buf_head=(g_buf_head+1)&1023;
	 		}
	 		while( ((g_buf_head-g_buf_tail)&1023)>=16)
	 		{
#ifdef X86_SIM
				printf("current header:%02x, tail:%d,head:%d\n",(unsigned char)g_buf[g_buf_tail],g_buf_tail, g_buf_head);
#endif 	 		
	 			if(g_buf[g_buf_tail]!=(char)UART_HEADER)
	 			{
	 				g_buf_tail=(g_buf_tail+1)&1023;
	 				continue;
	 			}
	 			for(int i=0;i<16;i++)
	 				g_temp[i]=g_buf[(g_buf_tail+i)&1023];
	 			if(!check_uart_cmd(g_temp))
	 			{
	 				g_buf_tail=(g_buf_tail+1)&1023;
	 				continue;
	 			}
#ifdef X86_SIM
				printf("valid uart packet in circular buffer\n");
				print_uart_packet(g_temp);
				printf("packet len:%d\n",ret);
#endif	 				
	 			switch(g_temp[1])
		 		{
		 		case 2:
		 			g_state.relay_mask=byte_to_int(g_temp+2);		 			
		 			break;
		 		case 3:
		 			g_state.temperature=byte_to_int(g_temp+2);
		 			g_state.wetness=byte_to_int(g_temp+4);
		 			if(!is_init_sent)
		 			{
		 				is_init_sent=true;
		 				memcpy(g_udp,UDP_HEADER,3);
					 	g_udp[3]=1;
					 	g_udp[4]=0;
					 	int_to_byte(g_state.relay_mask,g_udp+5);
						int_to_byte(g_state.temperature,g_udp+7);
						int_to_byte(g_state.wetness,g_udp+9);
						send_udp(g_udp,32,false);
		 			}		 			
		 			break;
		 		default:
		 			;
		 		}
#ifdef X86_SIM		 		
		 		print_state(&g_state, "state\n");
#endif		 		
	 			g_buf_tail=(g_buf_tail+16)&1023;	 			
	 		}
        }
        
    } 

	close(g_sock);
	close(g_uart);
	return 0;
}

#define PORT	3501
int send_udp(char *buf, int len, bool waitResponse)
{
#ifdef X86_SIM
	printf("send to server\n");
#endif
	if(-1==(sendto(g_sock,buf,len,0, (struct sockaddr *)&g_server_addr,g_server_addr_len)))
    {
    	// close(g_sock);
    	perror("fail to send packet:");
    	return -1;
    }	
	return 0;
}

void process_udp_cmd(char *buf, int len)
{
#ifdef X86_SIM
	printf("process udp cmd\n");
#endif
	if(buf[3]!=1)
		return; // not for rpi
	if(buf[4]!=1)
		return; // only process command 1 for now
	switch(buf[5])
	{
	case 1:
#ifdef X86_SIM
		printf("set relay cmd\n");
#endif	
		// forword to arduino
		init_uart_cmd(g_temp);
		g_temp[1]=1;
		g_temp[2]=buf[6];
		g_temp[3]=buf[7];
		fill_uart_cmd_check(g_temp);
		write(g_uart,g_temp,16);
		break;
	case 2:
#ifdef X86_SIM
		printf("read relay cmd\n");
#endif		
		// send response
		memcpy(g_udp,UDP_HEADER,3);
		g_udp[3]=1;
		g_udp[4]=1;
		g_udp[5]=2;
		int_to_byte(g_state.relay_mask,g_udp+6);
		send_udp(g_udp,32,false);
		// forward to arduino
		init_uart_cmd(g_temp);
		g_temp[1]=2;
		fill_uart_cmd_check(g_temp);
		write(g_uart,g_temp,16);
		break;
	case 3:
#ifdef X86_SIM
		printf("read sensor cmd\n");
#endif		
		// send response
		memcpy(g_udp,UDP_HEADER,3);
		g_udp[3]=1;
		g_udp[4]=1;
		g_udp[5]=3;
		int_to_byte(g_state.temperature,g_udp+6);
		int_to_byte(g_state.wetness,g_udp+8);
		send_udp(g_udp,32,false);
		// forward to arduino
		init_uart_cmd(g_temp);
		g_temp[1]=3;
		fill_uart_cmd_check(g_temp);
		write(g_uart,g_temp,16);
		break;
	case 4:
#ifdef X86_SIM
		printf("fish food cmd\n");
#endif		
		// forword to arduino
		init_uart_cmd(g_temp);
		g_temp[1]=4;
		fill_uart_cmd_check(g_temp);
		write(g_uart,g_temp,16);
		break;
	default:
		;
	}
}
