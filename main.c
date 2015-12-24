#include "main.h"

pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
LIST_HEAD(incomming_head);
int main (int argc, char *argv[])  
{  
		pthread_t acceptid, commutid;
		int sfd;
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
				if( (n = epoll_wait(efd, events, MAXEVENTS, -1)) == -1)
						if( (errno == EINTR) || (errno == EAGAIN) )
								continue;
				syslog(LOG_INFO, "%s %d Epoll wait count[%d]\n", __FILE__, __LINE__, n);
				for(i = 0; i < n; i++)  
				{  
						syslog(LOG_INFO, "%s %d Events[%d].events = [0x%X]\n", __FILE__, __LINE__, n, events[i].events);
						if(sfd == events[i].data.fd)  
						{  
								if( (result = pthread_create(&acceptid, NULL, accept_client, 
																(void *)events[i].data.fd)) != 0)
										syslog(LOG_ERR, "%s %d Incomming event[%d]'s FD is [%d],"
														" Creating accept thread error %s\n", 
														__FILE__, __LINE__, i, events[i].data.fd, strerror(result));
								else
										syslog(LOG_INFO, "%s %d Incomming event[%d]'s FD is [%d],"
														" Created accept thread ID [%lu]\n",
														__FILE__, __LINE__, n, events[i].data.fd, acceptid);
								continue;  
						}  
						else if(events[i].events & EPOLLRDHUP)
						{
								if( (result = pthread_create(&acceptid, NULL, close_client, 
																(void *)events[i].data.fd)) != 0)
								{
										syslog(LOG_ERR, "%s %d Catach EPOLLRDHUP events[0x%X]'s FD is [%d],"
														" Creating close thread error %s\n", 
														__FILE__, __LINE__, i, events[i].data.fd, strerror(result));
										close_client((void *)events[i].data.fd);  
								}
								else
								{
										syslog(LOG_INFO, "%s %d Catch EPOLLRDHUP events[0x%X]'s FD is [%d],"
														" Created close thread ID [%lu]\n",
														__FILE__, __LINE__, n, events[i].data.fd, acceptid);
								}
								continue;
						}
						else if(events[i].events & EPOLLIN) 
						{  
								if( (result = pthread_create(&commutid, NULL, opera_data, 
																(void *)events[i].data.fd)) !=0)
								{
										syslog(LOG_ERR, "%s %d Incomming event[%d]'s FD is [%d],"
														" Creating thread opera error, closing it %s\n", 
														__FILE__, __LINE__, i, events[i].data.fd, strerror(result));
										close (events[i].data.fd);  
										continue;  
								}
								syslog(LOG_INFO, "%s %d Incomming event[%d]'s FD is [%d], Created thread ID [%lu]\n",
												__FILE__, __LINE__, n, events[i].data.fd, acceptid);
						}  
						else
						{  /*
								syslog(LOG_ERR, "%s %d Catch epoll error, closing FD[%d]\n", __FILE__, __LINE__,
												events[i].data.fd); 
								close(events[i].data.fd);  
								continue; */ 
						}  
				}  
		}  
		free(events);  
		close(sfd);  
		return EXIT_SUCCESS;  
}  
