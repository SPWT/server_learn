#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void)
{
	int sfd;
	struct sockaddr_in con_addr;
	char rcv_buf[128] = {0};
	
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("socket");
		return -1;
	}
	
	memset(&con_addr, 0, sizeof(con_addr));
	con_addr.sin_family = AF_INET;
	con_addr.sin_port = htons(8000);
	con_addr.sin_addr.s_addr = inet_addr("192.168.240.160");

	do {
		if (connect(sfd, (struct sockaddr *)&con_addr, sizeof(struct sockaddr)) == -1)
		{
			perror("connect");
			break;
		}
		
		if (send(sfd, "HELLO", 5, 0) < 0)
		{
			perror("send");
			break;
		}
		if (recv(sfd, rcv_buf, sizeof(rcv_buf), 0) < 0)
		{
			perror("recv");
			break;
		}
		printf("[recv]:%s\n", rcv_buf);
	}while(0);
	
	close(sfd);
	
	return 0;
}
