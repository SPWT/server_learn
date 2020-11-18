#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define LISTEN_BACKLOG    1024
#define EPOLL_SIZE        1024

#if 1 /* select/epoll */
int main(void)
{
	int sfd, cfd;
	int sin_size;
	int rcv_len = 0;
	char rcv_buf[128] = {0};
	struct sockaddr_in my_addr, client_addr;
	
	int nready;
	int max_fd;
	fd_set rset, rfds;
	
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("socket");
		return -1;
	}
	
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8000);
	my_addr.sin_addr.s_addr = inet_addr("192.168.240.160");//INADDR_ANY
	
	do {
		if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
		{
			perror("bind");
			break;
		}
		
		if (listen(sfd, LISTEN_BACKLOG) == -1)
		{
			perror("listen");
			break;
		}
#if 0	/* select */
		FD_ZERO(&rfds);
		FD_SET(sfd, &rfds);
		max_fd = sfd;
		
		while (1)
		{
			rset = rfds;
			
			nready = select(max_fd + 1, &rset, NULL, NULL, NULL);
			
			if (nready < 0)
			{
				perror("select");
				continue;
			}

			if (FD_ISSET(sfd, &rset))
			{
				sin_size = sizeof(struct sockaddr_in);
				memset(client_addr, 0, sizeof(client_addr));
				cfd = accept(sfd, (struct sockaddr *)&client_addr, &sin_size);
				if (cfd == -1)
				{
					perror("accept");
					break;
				}
				printf("Client IP:%s\n", inet_ntoa(client_addr.sin_addr));
				
				if (max_fd == FD_SETSIZE)
				{
					printf("Out of range\n");
				}
				
				FD_SET(cfd, &rfds);
				if (cfd > max_fd)
				{
					max_fd = cfd;
				}
				
				if (--nready == 0) continue;
			}
			
			int i;
			for (i = sfd+1; i < max_fd + 1; i++)
			{
				if (FD_ISSET(i, &rset))
				{
					memset(rcv_buf, 0, sizeof(rcv_buf));
					rcv_len = recv(i, rcv_buf, sizeof(rcv_buf), 0);
					if (rcv_len > 0)
					{
						if (send(cfd, rcv_buf, rcv_len, 0) < 0)
						{
							perror("send");
							break;
						}
					}
					else if (rcv_len == 0)
					{
						FD_CLR(i, &rfds);
						close(i);
						break;
					}
					else
					{
						if (errno == EAGAIN || errno == EWOULDBLOCK)
						{
							printf("read all data");
						}
						
						FD_CLR(i, &rfds);
						close(i);
					}	

					if (--nready == 0) break;
				}
			}
		}
#else /* epoll */
		int epoll_fd;
		struct epoll_event ev, events[EPOLL_SIZE] = {0};
		int i;
	
		epoll_fd = epoll_create(1);
		
		ev.events = EPOLLIN;
		ev.data.fd = sfd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1)
		{
			perror("epoll_ctl:listen");
			break;
		}
		
		while (1)
		{
			nready = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
			
			for (i = 0; i < nready; i++)
			{
				if (events[i].data.fd == sfd)
				{
					sin_size = sizeof(struct sockaddr_in);
					memset(&client_addr, 0, sizeof(client_addr));
					cfd = accept(sfd, (struct sockaddr *)&client_addr, &sin_size);
					if (cfd == -1)
					{
						perror("accept");
						break;
					}
					printf("Client IP:%s\n", inet_ntoa(client_addr.sin_addr));
	
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = cfd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev) == -1)
					{
						perror("epoll_ctl:accept");
						break;
					}
					
				}
				else
				{
					int clinet_fd = events[i].data.fd;
					
					memset(rcv_buf, 0, sizeof(rcv_buf));
					rcv_len = recv(clinet_fd, rcv_buf, sizeof(rcv_buf), 0);
					if (rcv_len > 0)
					{
						if (send(cfd, rcv_buf, rcv_len, 0) < 0)
						{
							perror("send");
							break;
						}
					}
					else if (rcv_len == 0)
					{
						close(clinet_fd);
						ev.events = EPOLLIN | EPOLLET;
						ev.data.fd = clinet_fd;
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clinet_fd, &ev);
						break;
					}
					else
					{
						if (errno == EAGAIN || errno == EWOULDBLOCK)
						{
							printf("read all data");
						}
						ev.events = EPOLLIN | EPOLLET;
						ev.data.fd = clinet_fd;
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clinet_fd, &ev);
						close(clinet_fd);
					}	
					

				}
				
			}
		}
#endif
	} while(0);
	
	close(sfd);
	
	return 0;
}
#else
int main(void)
{
	int sfd, cfd;
	int sin_size;
	int rcv_len = 0;
	char rcv_buf[128] = {0};
	struct sockaddr_in my_addr, client_addr;
	
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("socket");
		return -1;
	}
	
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8000);
	my_addr.sin_addr.s_addr = inet_addr("192.168.240.160");//INADDR_ANY
	
	do {
		if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
		{
			perror("bind");
			break;
		}
		
		if (listen(sfd, LISTEN_BACKLOG) == -1)
		{
			perror("listen");
			break;
		}
		
		sin_size = sizeof(struct sockaddr_in);
		cfd = accept(sfd, (struct sockaddr *)&client_addr, &sin_size);
		if (cfd == -1)
		{
			perror("accept");
			break;
		}
		printf("Client IP:%s\n", inet_ntoa(client_addr.sin_addr));
		
		while ((rcv_len = recv(cfd, rcv_buf, sizeof(rcv_buf), 0)) > 0)
		{
			printf("[recv]:%s\n", rcv_buf);
			if (send(cfd, rcv_buf, rcv_len, 0) < 0)
			{
				perror("send");
				break;
			}
			memset(rcv_buf, 0, sizeof(rcv_buf));
		}
	} while(0);
	
	close(cfd);
	close(sfd);
	
	return 0;
}
#endif
