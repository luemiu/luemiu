#include "../include/main.h"

char * get_date_time(unsigned long threadid)
{
		static char str[128];
		time_t t = time(NULL);
		snprintf(str, sizeof str, "0x%lX %.24s", threadid, ctime(&t));
		return str;
}

void * Malloc(size_t size)
{
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d Malloc init size[%d]\n", __FILE__, __func__, __LINE__, size);
#endif
		return malloc(size);
}

void usage(void)
{
		fprintf(stdout, "Usage:%s [-d | -h]\n", APPNAME);
		fprintf(stdout, "      -d Daemon init\n");
		fprintf(stdout, "      -h Show this help message\n");
		exit(0);
}

int optargs(int argc, char ** argv)
{
		int ch;
		int args = 0;
		while ((ch = getopt(argc, argv, "dh")) != -1)
				switch(ch)
				{
						case 'd':
								args |= 1;
								break;
						case 'h':
								usage();
								break;
						default:
								usage();
								break;
				}
		return args;
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
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__,  __LINE__, strerror(ret));
				return -1;
		}
		for(rp = result; rp != NULL; rp = rp->ai_next)
		{
				if( (sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
						continue;
				if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
						break;
				close(sfd);
		}
		if(rp == NULL)
		{
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__, __LINE__, strerror(errno));
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
		while(1)
		{
				int infd;
				struct sockaddr inaddr;
				char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
				socklen_t len = sizeof inaddr;
				if( (infd = accept(sfd, &inaddr, &len)) == -1)
				{
						if( (errno != EAGAIN) || (errno != EWOULDBLOCK) )
								syslog(LOG_ERR, "%s %s %d Thread[0x%lX] accept %s\n",
												__FILE__, __func__,  __LINE__, tid, strerror(errno));
						break;
				}
				if(getnameinfo(&inaddr, len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
				{
						syslog(LOG_ERR, "%s %s %d Thread[0x%lX] getnameinfo from FD[%d] %s, closing it\n",
										__FILE__, __func__,  __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				if(set_nonblocking(infd) == -1)
				{
						syslog(LOG_ERR, "%s %s %d Thread[0x%lX] set nonblocking FD[%d] %s, closing it\n", 
										__FILE__, __func__,  __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				event.data.fd = infd;
				event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLPRI | EPOLLRDHUP;
				if(epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event) == -1)	
				{
						syslog(LOG_ERR, "%s %s %d Thread[0x%lX] EPOLL_CTL_ADD FD[%d] %s, closing it\n",
										__FILE__, __func__,  __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
		}
		return NULL;
}


void * commut_thread(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		char * result;
		int ret;
		struct list_head * listtmp, *ntmp;
		struct client * sctmp = NULL;
		if( (ret =pthread_mutex_lock(&head_mutex)) != 0)
				syslog(LOG_DEBUG, "%s %s %d Thread[0x%lX] lock %s\n",
								__FILE__, __func__,  __LINE__, tid, strerror(ret));
		list_for_each_safe(listtmp, ntmp, &head)
		{
				sctmp = list_entry(listtmp, struct client, list);
				if(sctmp->readed)
						continue;
				int commutfd = sctmp->cevent.data.fd;
				while(1)
				{
						ssize_t count;
						char buf[MAXEVENTS];
						if( (count = read(commutfd, buf, sizeof buf)) == -1)
						{
								if(EBADF == errno)
										pthread_exit(0);
								if(EAGAIN != errno)
										syslog(LOG_ERR, "%s %s %d Thread[0x%lX] read FD[%d] %s\n",
														__FILE__, __func__,  __LINE__, tid, commutfd,  strerror(errno));
								break;
						}
						else if(count == 0)
						{
								break;
						}
						switch(buf[0])
						{
								case '1':
								case '2':
										result = get_date_time(tid);
										break;
								default:
										break;
						}
						if(write(commutfd, result, strlen(result)) == -1)
						{
								syslog(LOG_ERR, "%s %s %d Thread[0x%lX] write FD[%d] %s\n",
												__FILE__,__func__,  __LINE__, tid, commutfd,  strerror(errno));
								pthread_exit(0);
						}
				} // end while
				sctmp->readed = 1;
				sctmp = NULL;
		} // end list for each
		if( (ret = pthread_mutex_unlock(&head_mutex)) != 0)
				syslog(LOG_DEBUG, "%s %s %d Thread[0x%lX] unlock %s\n",
								__FILE__, __func__,  __LINE__, tid, strerror(ret));
		return NULL;
}

void * free_thread(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		struct list_head * listclient, * ntmp;
		struct client * sctmp = NULL;
		int ret;
		while(1)
		{
				if(list_empty(&head))
				{
						sleep(1);
						continue;
				}
				if( (ret = pthread_mutex_lock(&head_mutex)) != 0)
				{
						syslog(LOG_DEBUG, "%s %s %d Thread[0x%lX] lock FD[%d] %s\n",__FILE__, __func__,  
										__LINE__, tid, sctmp->cevent.data.fd, strerror(ret));
						continue;
				}
				listclient = head.next;
				sctmp = list_entry( listclient, struct client, list);
				if(sctmp->readed == 1)
				{
						close(sctmp->cevent.data.fd);
						list_del(&sctmp->list);
						free(sctmp);
				}
				if( (ret = pthread_mutex_unlock(&head_mutex)) != 0)
						syslog(LOG_DEBUG, "%s %s %d Thread[0x%lX] lock FD[%d] %s\n",__FILE__, __func__,  
										__LINE__, tid, sctmp->cevent.data.fd, strerror(ret));
		}
		return NULL;
}
