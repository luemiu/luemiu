#include "main.h"

LIST_HEAD(head);
LIST_HEAD(dog);
pthread_mutex_t dog_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dog_cond = PTHREAD_COND_INITIALIZER;
int main (int argc, char *argv[])  
{  
		pthread_t ctid;//operation client data thread
		if (argc != 2)  
		{  
				fprintf (stderr, "Usage: %s [port]\n", argv[0]);  
				return EXIT_FAILURE;
		}  
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Application init!");
		if( (sfd = bind_sock(argv[1])) == -1)
				return EXIT_FAILURE;
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Socket inited!");
		if(set_nonblocking(sfd) == -1)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Socket set non-blocking!");
		if(listen (sfd, SOMAXCONN) == -1)  
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Socket listened!");
		if( (efd = epoll_create1 (0)) == -1)  
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Epoll created!");
		event.data.fd = sfd;  
		event.events = EPOLLIN | EPOLLET;
		if(epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) == -1)
		{
				syslog(LOG_ERR, "%s %d EPOLL_CTL_ADD error %s\n", __FILE__, __LINE__, strerror(errno));
				close(efd);
				close(sfd);
				return EXIT_FAILURE;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Epoll EPOLL_CTL_ADD added!");
		if( (events = (struct epoll_event *)calloc (MAXEVENTS, sizeof event)) == NULL)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, "Can not calloc!");
				close(efd);
				close(sfd);
				return EXIT_FAILURE;
		}
		while(1)  
		{  
				int n, i, result;  
				int opflag = 0;
				if( (n = epoll_wait(efd, events, MAXEVENTS, -1)) == -1)
						if( (errno == EINTR) || (errno == EAGAIN) )
								continue;
				syslog(LOG_INFO, "%s %d Epoll wait count[%d]\n", __FILE__, __LINE__, n);
				for(i = 0; i < n; i++)  
				{  
						syslog(LOG_INFO, "%s %d Events[%d].events = [0x%X] insert event to head\n",
										__FILE__, __LINE__, n, events[i].events);
						if(events[i].data.fd == sfd)
						{
								if( (result = pthread_create(&ctid, NULL, accept_thread, NULL)) != 0)
								{
										syslog(LOG_INFO, "%s %d event[%d] accept thread create error %s\n",
														__FILE__, __LINE__, n, strerror(result));
								}
								else
										syslog(LOG_INFO, "%s %d event[%d] accept thread created\n",
														__FILE__, __LINE__, n );
								continue;
						}
						if( (events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP)
										|| (events[i].events & EPOLLERR) || (events[i].events & EPOLLPRI) )
						{
								syslog(LOG_INFO, "%s %d EPOLLRDHUP[0x%X]EPOLLHUP[0x%X]EPOLLERR[0x%X]EPOLLPRI[0x%X]\n",
												__FILE__, __LINE__, events[i].events & EPOLLRDHUP,
												events[i].events & EPOLLHUP, events[i].events & EPOLLERR,
												events[i].events & EPOLLPRI);
								struct dog * sc = (struct dog *)malloc(sizeof(struct dog));
								if(sc == NULL)
								{
										syslog(LOG_ERR, "%s %d Malloc events[%d].events error\n", __FILE__, __LINE__, i);
										close(events[i].data.fd);
								}
								sc->fd = events[i].data.fd;
								pthread_mutex_lock(&dog_mutex);
								list_add_tail(&sc->list, &dog);	
//								pthread_cond_wait(&dog_cond, &dog_mutex);
								pthread_mutex_unlock(&dog_mutex);
								continue;
						}
						if(events[i].events & EPOLLIN)
						{
								struct client * sc = (struct client *)malloc(sizeof(struct client));
								if(sc == NULL)
								{
										syslog(LOG_ERR, "%s %d Malloc events[%d].events error\n", __FILE__, __LINE__, i);
										close(events[i].data.fd);
								}
								sc->cevent = events[i];
								sc->readed = 0;
								pthread_mutex_lock(&head_mutex);
								list_add_tail(&sc->list, &head);	
								pthread_mutex_unlock(&head_mutex);
								opflag = 1;
						}
				} // end for
				if(opflag)
				{
						if( (result = pthread_create(&ctid, NULL, commut_thread, NULL)) != 0)
						{
								syslog(LOG_INFO, "%s %d commut thread create error %s, conntinue\n",
												__FILE__, __LINE__, strerror(result));
								continue;
						}
						syslog(LOG_INFO, "%s %d commut thread[%lu] created\n", __FILE__, __LINE__, ctid);
						if( (result = pthread_create(&ctid, NULL, dog_thread, NULL)) != 0)
						{
								syslog(LOG_INFO, "%s %d Dog thread create error %s\n",
												__FILE__, __LINE__, strerror(result));
						}
						syslog(LOG_INFO, "%s %d Dog thread[%lu] created\n", __FILE__, __LINE__, ctid);
				}
		} // end while 
		free(events);  
		close(sfd);  
		return EXIT_SUCCESS;  
}  
