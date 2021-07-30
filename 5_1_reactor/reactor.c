#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_LENGTH      1024
#define MAX_EPOLL_EVENTS   1024

typedef int CALLBACK(int, int, void *);

int recv_cb(int fd, int events, void *arg);
int send_cb(int fd, int events, void *arg);

struct ntyevent {
	int fd;
	int events;
	
	void *arg;
	int (*callback)(int fd, int events, void *arg);
	
	char buffer[BUFFER_LENGTH];
	int length;

	int status;
};

struct ntyreactor {
	int epfd;
	struct ntyevent *events;
};

void nty_event_set(struct ntyevent *ev, int fd, int events, CALLBACK *callback, void *arg)
{
	ev->fd = fd;
	ev->events = events;
	ev->arg = arg;
	ev->callback = callback;
}

int nty_event_add(int epfd, struct ntyevent *ev)
{
	struct epoll_event ep_ev = {0, {0}};
	int op;
	
	ep_ev.data.ptr = ev;
	ep_ev.events = ev->events;

	if (ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}
	else
	{
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if (epoll_ctl(epfd, op, ev->fd, &ep_ev) < 0)
	{
		perror("epoll_ctl:add");
		return -1;
	}
	
	return 0;
}

int nty_event_del(int epfd, struct ntyevent *ev)
{
	struct epoll_event ep_ev = {0, {0}};

	if (ev->status != 1)
	{
		return -1;
	}

	ep_ev.data.ptr = ev;
	ev->status = 0;

	epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &ep_ev);

	return 0;
}

int ntyreactor_destroy(struct ntyreactor *reactor)
{
	close(reactor->epfd);
	if (reactor->events != NULL)
	{
		free(reactor->events);
		reactor->events = NULL;
	}

	return 0;
}

int ntyreactor_init(struct ntyreactor *reactor)
{
	if (reactor == NULL)
	{
		return -1;
	}
	memset(reactor, 0, sizeof(struct ntyreactor));

	reactor->epfd = epoll_create(1);
	if (reactor->epfd <= 0)
	{
		perror("epoll create");
		return -2;
	}

	reactor->events = (struct ntyevent*)malloc(MAX_EPOLL_EVENTS * sizeof(struct ntyevent));
	if (reactor->events == NULL)
	{
		close(reactor->epfd);
		return -3;
	}

	return 0;
}

int ntyreactor_run(int epfd, struct ntyreactor *reactor)
{
	int nready;
	struct epoll_event events[MAX_EPOLL_EVENTS] = {0};
	int i;

	while (1)
	{
		nready = epoll_wait(reactor->epfd, events, MAX_EPOLL_EVENTS, -1);

		if (nready < 0)
		{
			perror("epoll_wait");
			continue;
		}

		for (i = 0; i < nready; i++)
		{
			struct ntyevent *ev;
			ev = (struct ntyevent *)events[i].data.ptr;

			if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
			{
				ev->callback(ev->fd, ev->events, ev->arg);
			}
			if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
			{
				ev->callback(ev->fd, ev->events, ev->arg);
			}
		}
	}

	return 0;
}

int send_cb(int fd, int events, void *arg)
{
	struct ntyreactor *reactor = (struct ntyreactor *)arg;
	struct ntyevent *ev = &reactor->events[fd];
	int len;

	len = send(fd, ev->buffer, sizeof(ev->buffer), 0);
	nty_event_del(reactor->epfd, ev);

	if (len < 0)
	{
		printf("send to [%d] error\n", fd);
	}
	else
	{
		nty_event_set(ev, fd, EPOLLIN, recv_cb, reactor);
		nty_event_add(reactor->epfd, ev);
	}

	return len;
}

int recv_cb(int fd, int events, void *arg)
{
	struct ntyreactor *reactor = (struct ntyreactor *)arg;
	struct ntyevent *ev = &reactor->events[fd];
	int len;

	memset(ev->buffer, 0, sizeof(ev->buffer));
	len = recv(fd, ev->buffer, sizeof(ev->buffer), 0);
	nty_event_del(reactor->epfd, ev);

	if (len < 0)
	{
		printf("[%d]recv error\n", fd);
		close(fd);
	}
	else if (len == 0)
	{
		close(fd);
		printf("[%d]close\n", fd);
	}
	else
	{
		printf("[%d]recv:%s\n", fd, ev->buffer);

		nty_event_set(ev, fd, EPOLLOUT, send_cb, reactor);
		nty_event_add(reactor->epfd, ev);
	}

	return len;
}

int accept_cb(int fd, int events, void *arg)
{
	struct ntyreactor *reactor;
	struct sockaddr_in client_addr;
	int sin_size;
	int cfd;

	reactor = (struct ntyreactor *)arg;
	if (reactor == NULL) return -1;

	memset(&client_addr, 0, sizeof(client_addr));
	sin_size = sizeof(client_addr);

	cfd = accept(fd, (struct sockaddr *)&client_addr, &sin_size);
	if (cfd < 0)
	{
		perror("accept");
		return -1;
	}
	printf("Client IP:%s\n", inet_ntoa(client_addr.sin_addr));

//	int flag = 0;
//	if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {
//		printf("%s: fcntl nonblocking failed, %d\n", __func__, MAX_EPOLL_EVENTS);
//		return -2;
//	}

	nty_event_set(&reactor->events[cfd], cfd, EPOLLIN, recv_cb, reactor);
	nty_event_add(reactor->epfd, &reactor->events[cfd]);

	return 0;
}

int ntyreactor_addlistener(int sockfd, struct ntyreactor *reactor, CALLBACK *acceptor)
{
	if (reactor == NULL) return -1;
	if(reactor->events == NULL) return -2;

	nty_event_set(&reactor->events[sockfd], sockfd, EPOLLIN, acceptor, reactor);
	nty_event_add(reactor->epfd, &reactor->events[sockfd]);

	return 0;
}

int init_sock(int port)
{
	int sfd;
	struct sockaddr_in my_addr;

	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("socket");
		return -1;
	}

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("192.168.240.160");

	if (bind(sfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("bind");
		return -1;
	}

	if (listen(sfd, 20) < 0) {
		perror("listen");
	}

	return sfd;
}

int main(void)
{
	struct ntyreactor reactor = {0, 0};
	int port = 8000;

	int sockfd = init_sock(port);

	ntyreactor_init(&reactor);
	ntyreactor_addlistener(sockfd, &reactor, accept_cb);
	ntyreactor_run(reactor.epfd, &reactor);
	ntyreactor_destroy(&reactor);

	close(sockfd);

	return 0;
}
