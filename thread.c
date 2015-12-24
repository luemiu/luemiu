#include "main.h"

void * thread_accept(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		int sfd = (int) data;
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

void * thread_commut(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		int done = 0;
		char * result;
		char  msg[] = "this is test message";
		int commutfd = (int)data;
		syslog(LOG_INFO, "%s %d Thread commut[%lu] init, FD[%d]!\n", __FILE__, __LINE__, tid, commutfd);
		while(1)
		{
				ssize_t count;
				char buf[MAXBUFF];
				if( (count = read(commutfd, buf, sizeof buf)) == -1)
				{
						if(errno != EAGAIN)
						{
								syslog(LOG_ERR, "%s %d Thread commut[%lu] read FD[%d] error %s\n",
												__FILE__, __LINE__, tid, commutfd,  strerror(errno));
								done = 1;
						}
						break;
				}
				else if(count == 0)
				{
						done = 1;
						break;
				}
				syslog(LOG_INFO, "%s %d Thread commut[%lu] reading [%d] chars from FD[%d]!\n", 
								__FILE__, __LINE__, tid, count, commutfd);
				switch(buf[0])
				{
						case '1':
								result = get_date_time();
								break;
						case '2':
								result = msg;
								break;
						default:
								break;
				}
				if(write(commutfd, result, strlen(result)) == -1)
				{
						syslog(LOG_ERR, "%s %d Thread commut[%lu] write FD[%d] error %s\n",
										__FILE__, __LINE__, tid, commutfd,  strerror(errno));
						close(commutfd);
						break;
				}
				syslog(LOG_INFO, "%s %d Thread commut[%lu] writting FD[%d]!\n", __FILE__, __LINE__, tid, commutfd);
		}
		if(done)
		{
				syslog(LOG_INFO, "%s %d Thread commut[%lu] read and write FD[%d] finished, closing it,"
								" thread finished\n", __FILE__, __LINE__, tid, commutfd);
				close(commutfd);
		}
		return NULL;
}
