#include "main.h"

static char str[64];

char * get_date_time(void)
{
		time_t t = time(NULL);
		snprintf(str, sizeof str, "%.24s\r\n", ctime(&t));
		return str;
}

int bind_sock(char * port)
{
		struct addrinfo inaddr;
		struct addrinfo * result, *rp;

		memset(&inaddr, 0, sizeof(struct addrinfo));
		inaddr.ai_family = AF_UNSPEC;
		inaddr.ai_socktype = SOCK_STREAM;
		inaddr.ai_flags = AI_PASSIVE;

		int sfd, ret;

		if( (ret = getaddrinfo(NULL, port, &inaddr, &result)) != 0)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(ret));
				return -1;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Get addr info done");
		for(rp = result; rp != NULL; rp = rp->ai_next)
		{
				if( (sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
						continue;
				if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
						break;
				syslog(LOG_ERR, "%s %d Socket and bind init error %s\n", __FILE__, __LINE__, strerror(errno));
				close(sfd);
		}
		if(rp == NULL)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(errno));
				return -1;
		}
		freeaddrinfo(result);
		return sfd;
}

int set_nonblocking(int sfd)
{
		int flags;
		if( (flags = fcntl(sfd, F_GETFL, 0)) == -1)
				return -1;
		flags |= O_NONBLOCK;
		if(fcntl(sfd, F_SETFL, flags))
				return -1;
		return 0;
}

void * accept_thread(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		syslog(LOG_INFO, "%s %d Thread accept[%lu] init!\n", __FILE__, __LINE__, tid);
		while(1)
		{
				int infd;
				struct sockaddr inaddr;
				char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
				socklen_t len = sizeof inaddr;
				if( (infd = accept(sfd, &inaddr, &len)) == -1)
				{
						if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
								break;
						else
						{
								syslog(LOG_ERR, "%s %d Thread accept[%lu] accept error %s\n",
												__FILE__, __LINE__, tid, strerror(errno));
								break;
						}
				}
				syslog(LOG_INFO, "%s %d Incomming client, thread accept[%lu] accept in client FD[%d]\n", 
								__FILE__, __LINE__, tid, infd);
				if(getnameinfo(&inaddr, len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
				{
						syslog(LOG_ERR, "%s %d Thread accept[%lu], getnameinfo from FD[%d] error %s, closing it\n",
										__FILE__, __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				syslog(LOG_INFO, "%s %d Thread accept[%lu] getting client FD[%d] info, addr %s port %s\n", 
								__FILE__, __LINE__, tid, infd, hbuf, sbuf);
				if(set_nonblocking(infd) == -1)
				{
						syslog(LOG_ERR, "%s %d Thread accept[%lu] set nonblocking FD[%d] error, closing it%s\n", 
										__FILE__, __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				syslog(LOG_INFO, "%s %d Thread accept[%lu] set non-blocking FD[%d] finished!\n", 
								__FILE__, __LINE__, tid, infd);
				event.data.fd = infd;
				event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLPRI | EPOLLRDHUP;
				if(epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event) == -1)	
				{
						syslog(LOG_ERR, "%s %d Thread accpet[%lu] EPOLL_CTL_ADD FD[%d] error, closing it%s\n",
										__FILE__, __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				syslog(LOG_INFO, "%s %d Thread accept[%lu] EPOLL_CTL_ADD FD[%d] finished!\n", 
								__FILE__, __LINE__, tid, infd);
		}
		syslog(LOG_INFO, "%s %d Thread accept[%lu] end!\n", __FILE__, __LINE__, tid);
		return NULL;
}


void * commut_thread(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		syslog(LOG_INFO, "%s %d Thread commut[%lu] init!\n", __FILE__, __LINE__, tid);
		char * result;
		int ret;
		struct list_head * listtmp, *ntmp;
		struct client * sctmp;
		list_for_each_safe(listtmp, ntmp, &head)
		{
				sctmp = list_entry(listtmp, struct client, list);
				int commutfd = sctmp->cevent.data.fd;
				while(1)
				{
						ssize_t count;
						char buf[MAXEVENTS];
						if( (count = read(commutfd, buf, sizeof buf)) == -1)
						{
								if(EAGAIN != errno)
										syslog(LOG_ERR, "%s %d Thread commut[%lu] read FD[%d] error %s\n",
														__FILE__, __LINE__, tid, commutfd,  strerror(errno));
								break;
						}
						else if(count == 0)
						{
								break;
						}
						syslog(LOG_INFO, "%s %d Thread commut[%lu] reading [%d] chars from FD[%d]!\n", 
										__FILE__, __LINE__, tid, count, commutfd);
						switch(buf[0])
						{
								case '1':
								case '2':
										result = get_date_time();
										break;
								default:
										break;
						}
						if(write(commutfd, result, strlen(result)) == -1)
						{
								syslog(LOG_ERR, "%s %d Thread commut[%lu] write FD[%d] error %s\n",
												__FILE__, __LINE__, tid, commutfd,  strerror(errno));
								break;
						}
						syslog(LOG_INFO, "%s %d Thread commut[%lu] writting FD[%d]!\n", 
										__FILE__, __LINE__, tid, commutfd);
				} // end while
		} // end list for each
		return NULL;
}

void * close_thread(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		int cfd = (int) data;
		syslog(LOG_INFO, "%s %d Thread close[%lu] FD[%d] init!\n", __FILE__, __LINE__, tid, cfd);
		struct list_head * listtmp, * ntmp;
		struct client * sctmp;
		list_for_each_safe(listtmp, ntmp, &head)
		{
				sctmp = list_entry(listtmp, struct client, list);
				if(cfd == sctmp->cevent.data.fd)
				{
						list_del(&sctmp->list);
						close(sctmp->cevent.data.fd);
						free(sctmp);
						break;
				}
		}
		return NULL;
}
