#include "main.h"

void * accept_client(void * data)
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
				struct client_info * cd = (struct client_info *)malloc(sizeof(struct client_info));
				if(cd == NULL)
				{
						syslog(LOG_ERR, "%s %d Thread accpet[%lu] malloc  FD[%d] list error, closing it%s\n",
										__FILE__, __LINE__, tid, infd, strerror(errno));
						close(infd);
						break;
				}
				memset(cd, 0, sizeof(struct client_info));
				syslog(LOG_INFO, "%s %d Thread accept[%lu] malloc FD[%d] list finished!\n", 
								__FILE__, __LINE__, tid, infd);
				cd->clientfd = infd;
				list_add_tail(&cd->list, &incomming_head);
				syslog(LOG_INFO, "%s %d Thread accept[%lu] add FD[%d] to incomming list finished!\n", 
								__FILE__, __LINE__, tid, infd);
				struct list_head * listtmp;
				struct client_info * cdtmp;
				list_for_each(listtmp, &incomming_head)
				{
						cdtmp = (struct client_info *)list_entry(listtmp, struct client_info, list); 
						syslog(LOG_INFO, "%s %d Thread accept[%lu] list for each FD[%d]\n", 
										__FILE__, __LINE__, tid, cdtmp->clientfd);
				}
		}
		syslog(LOG_INFO, "%s %d Thread accept[%lu] end!\n", __FILE__, __LINE__, tid);
		return NULL;
}


void * opera_data(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		int done = 0;
		int exitit = 0;
		char * result;
		char  msg[] = "this is test message";
		int commutfd = (int)data;
		syslog(LOG_INFO, "%s %d Thread read[%lu] init, FD[%d]!\n", __FILE__, __LINE__, tid, commutfd);
		struct list_head * listtmp;
		struct client_info * cdtmp;
		list_for_each(listtmp, &incomming_head)
		{
				cdtmp = (struct client_info *)list_entry(listtmp, struct client_info, list); 
				if(cdtmp->clientfd == commutfd)
				{
						syslog(LOG_INFO, "%s %d Thread accept[%lu] found FD[%d] struct head_list\n", 
										__FILE__, __LINE__, tid, cdtmp->clientfd);
						exitit = 0;
						break;
				}
				else
						exitit = 1;
		}
		if(exitit)
		{
				syslog(LOG_INFO, "%s %d Thread accept[%lu] not found FD[%d] struct head_list, exit thread\n", 
								__FILE__, __LINE__, tid, cdtmp->clientfd);
				pthread_exit(0);
		}
		while(1)
		{
				ssize_t count;
				char buf[MAXBUFF];
				if( (count = read(commutfd, buf, sizeof buf)) == -1)
				{
						if(errno != EAGAIN)
						{
								syslog(LOG_ERR, "%s %d Thread read[%lu] read FD[%d] error %s\n",
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
				syslog(LOG_INFO, "%s %d Thread read[%lu] reading [%d] chars from FD[%d]!\n", 
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
						syslog(LOG_ERR, "%s %d Thread read[%lu] write FD[%d] error %s\n",
										__FILE__, __LINE__, tid, commutfd,  strerror(errno));
						done = 1;
						break;
				}
				syslog(LOG_INFO, "%s %d Thread read[%lu] writting FD[%d]!\n", __FILE__, __LINE__, tid, commutfd);
		}
		if(done)
		{
				syslog(LOG_INFO, "%s %d Thread read[%lu] read and write FD[%d] finished, closing it,"
								" thread finished\n", __FILE__, __LINE__, tid, commutfd);
				close(commutfd);
				list_for_each(listtmp, &incomming_head)
				{
						cdtmp = (struct client_info *)list_entry(listtmp, struct client_info, list); 
						syslog(LOG_INFO, "%s %d Thread read[%lu] 1 list for each FD[%d]\n", 
										__FILE__, __LINE__, tid, cdtmp->clientfd);
				}
		}
		return NULL;
}
void * close_client(void * data)
{
		pthread_t tid = pthread_self();
		pthread_detach(tid);
		struct list_head * listtmp;
		struct client_info * cdtmp;
		int closefd = (int) data;
		list_for_each(listtmp, &incomming_head)
		{
				cdtmp = (struct client_info *)list_entry(listtmp, struct client_info, list); 
				if(cdtmp->clientfd == closefd)
				{
						syslog(LOG_INFO, "%s %d Thread close[%lu] found FD[%d], closing it\n", 
										__FILE__, __LINE__, tid, cdtmp->clientfd);
						__list_del_entry(&cdtmp->list);
						free(cdtmp);
						close(closefd);
				}
				list_for_each(listtmp, &incomming_head)
				{
						cdtmp = (struct client_info *)list_entry(listtmp, struct client_info, list); 
						syslog(LOG_INFO, "%s %d Thread close[%lu] check FD[%d]\n", 
										__FILE__, __LINE__, tid, cdtmp->clientfd);
				}
		}
		return NULL;
}
