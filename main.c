#include "main.h"

LIST_HEAD(head);
pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;
int main (int argc, char *argv[])  
{ 
		if(optargs(argc, argv) & 1)
				daemon(0, 0);
		char port[] = "8008";
		pthread_t ctid;//operation client data thread
		openlog(APPNAME, LOG_ODELAY, LOG_LOCAL0);
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Application init!");
#endif
		if (signal(SIGSEGV, dump) == SIG_ERR)
				syslog(LOG_DEBUG, "%s %s %d Dump SIG_ERR\n", __FILE__, __func__, __LINE__);
		if( (sfd = bind_sock(port)) == -1)
				return EXIT_FAILURE;
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Socket inited!");
#endif
		if(set_nonblocking(sfd) == -1)
		{
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Socket set non-blocking!");
#endif
		if(listen (sfd, SOMAXCONN) == -1)  
		{
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Socket listened!");
#endif
		if( (efd = epoll_create1 (0)) == -1)  
		{
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__, __LINE__, strerror(errno));
				close(sfd);
				return EXIT_FAILURE;
		}
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Epoll created!");
#endif
		event.data.fd = sfd;  
		event.events = EPOLLIN | EPOLLET;
		if(epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) == -1)
		{
				syslog(LOG_ERR, "%s %s %d EPOLL_CTL_ADD %s\n", __FILE__, __func__, __LINE__, strerror(errno));
				close(efd);
				close(sfd);
				return EXIT_FAILURE;
		}
#if DEBUG
		syslog(LOG_DEBUG, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Epoll EPOLL_CTL_ADD added!");
#endif
		if( (events = (struct epoll_event *)MALLOC(MAXEVENTS*sizeof(event))) == NULL)
		{
				syslog(LOG_ERR, "%s %s %d %s\n", __FILE__, __func__, __LINE__, "Can not calloc!");
				close(efd);
				close(sfd);
				return EXIT_FAILURE;
		}
		int freeflag = 1;
		while(1)  
		{  
				int n, i, result;  
				int opflag = 0;
				if( (n = epoll_wait(efd, events, MAXEVENTS, -1)) == -1)
						if( (errno == EINTR) || (errno == EAGAIN) )
								continue;
				for(i = 0; i < n; i++)  
				{  
#if DEBUG
						syslog(LOG_DEBUG, "%s %s %d Events[%d] = [0x%X]\n",
										__FILE__, __func__, __LINE__, n, events[i].events);
#endif
						if(events[i].data.fd == sfd)
						{
								if( (result = pthread_create(&ctid, NULL, accept_thread, NULL)) != 0)
								{
										syslog(LOG_ERR, "%s %s %d event[%d] accept thread create %s\n",
														__FILE__, __func__, __LINE__, n, strerror(result));
								}
#if DEBUG
								else
										syslog(LOG_DEBUG, "%s %s %d event[%d] accept thread created\n",
														__FILE__, __func__, __LINE__, n );
#endif
								continue;
						}
						if( (events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP)
										|| (events[i].events & EPOLLERR) || (events[i].events & EPOLLPRI) )
						{
#if DEBUG
								syslog(LOG_ERR, "%s %s %d EPOLLRDHUP[0x%X]EPOLLHUP[0x%X]EPOLLERR[0x%X]EPOLLPRI[0x%X]\n",
												__FILE__, __func__, __LINE__, events[i].events & EPOLLRDHUP,
												events[i].events & EPOLLHUP, events[i].events & EPOLLERR,
												events[i].events & EPOLLPRI);
#endif
								continue;
						}
						if(events[i].events & EPOLLIN)
						{
								if( (result =pthread_mutex_lock(&head_mutex)) != 0)
										syslog(LOG_ERR, "%s %s %d Lock %s\n", 
														__FILE__, __func__, __LINE__, strerror(result));
								struct client * sc = (struct client *)malloc(1*sizeof(struct client));
								if(sc == NULL)
								{
										syslog(LOG_ERR, "%s %s %d Malloc events[%d]\n", __FILE__, __func__, __LINE__, i);
										close(events[i].data.fd);
										if( (result = pthread_mutex_unlock(&head_mutex)) != 0)
												syslog(LOG_ERR, "%s %s %d Unlock %s\n", 
																__FILE__, __func__, __LINE__, strerror(result));
										continue;
								}
#if DEBUG
								syslog(LOG_ERR, "%s %s %d Malloc events[%d] in addr[0x%lX]\n", 
												__FILE__, __func__, __LINE__, i, (unsigned long)sc);
#endif
								bzero(sc, sizeof(struct client));
								sc->cevent = events[i];
								sc->readed = 0;
								list_add_tail(&sc->list, &head);	
								sc = NULL;
								if( (result = pthread_mutex_unlock(&head_mutex)) != 0)
										syslog(LOG_ERR, "%s %s %d Unlock %s\n", 
														__FILE__, __func__, __LINE__, strerror(result));
								opflag = 1;
						}
				} // end for
				if(opflag)
				{
						if( (result = pthread_create(&ctid, NULL, commut_thread, NULL)) != 0)
						{
								syslog(LOG_DEBUG, "%s %s %d commut thread create %s, conntinue\n",
												__FILE__, __func__, __LINE__, strerror(result));
								continue;
						}
#if DEBUG
						syslog(LOG_DEBUG, "%s %s %d commut thread[0x%lX] created\n", __FILE__, __func__, __LINE__, ctid);
#endif
				}
				if(freeflag)
				{
						if( (result = pthread_create(&ctid, NULL, free_thread, NULL)) != 0)
						{
								syslog(LOG_DEBUG, "%s %s %d Free thread create %s\n",
												__FILE__, __func__, __LINE__, strerror(result));
								freeflag = 1;
						}
#if DEBUG
						syslog(LOG_DEBUG, "%s %s %d Free thread[0x%lX] created\n", __FILE__, __func__, __LINE__, ctid);
#endif
						freeflag = 0;
				}
		} // end while 
		free(events);  
		close(sfd);  
		closelog();
		return EXIT_SUCCESS;  
}  
