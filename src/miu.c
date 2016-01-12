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
				log("Malloc init size[%d]\n", size);
#endif
				return malloc(size);
}

void usage(void)
{
				fprintf(stdout, "Usage:%s -c public key file [-d] [-h] [-p] -r private key file\n", APPNAME);
				fprintf(stdout, "      -c Public key @file\n");
				fprintf(stdout, "      -d Daemon init\n");
				fprintf(stdout, "      -h Show this help message\n");
				fprintf(stdout, "      -p Server port\n");
				fprintf(stdout, "      -r Private key @file\n");
				exit(0);
}

int optargs(int argc, char ** argv)
{
				int ch;
				int args = 0;
				FILE * f;
				while ((ch = getopt(argc, argv, "c:dhp:r:")) != -1)
								switch(ch)
								{
												case 'c':
																if( (f = fopen(optarg, "r")) == NULL)
																{
																				log("Open file [%s] %s\n", optarg, strerror(errno));
																				exit(0);
																}
																fclose(f);
																args |= cflag;
																argpublic = optarg;	
																break;
												case 'd':
																args |= dflag;
																break;
												case 'h':
																usage();
																break;
												case 'p':
																if( (f = fopen(optarg, "r")) == NULL)
																{
																				log("Open file [%s] %s\n", optarg, strerror(errno));
																				exit(0);
																}
																fclose(f);
																args |= pflag;
																argport = optarg;	
																break;
												case 'r':
																args |= rflag;
																argprivate = optarg;	
																break;
												default:
																usage();
																break;
								}
				return args;
}

int bind_sock(char * port)
{
				struct addrinfo * result, *rp, hints;
				int ret, yes = 1;
				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_flags = AI_PASSIVE; 
				if( (ret = getaddrinfo(NULL, port, &hints, &result)) != 0)
				{
								log("getaddrinfo %s", gai_strerror(ret));
								return -1;
				}
				ret = 0;
				for(rp = result; rp != NULL; rp = rp->ai_next)
				{
								ret++;
								if( (sockfd[ret-1] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
								{
												log("socket %s", gai_strerror(errno));
												continue;
								}
								if (setsockopt(sockfd[ret-1], SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
								{
												log("setsockopt %s", gai_strerror(errno));
												close(sockfd[ret-1]);
												sockfd[ret-1] = -1;
												continue;
								}
								if(bind(sockfd[ret-1], rp->ai_addr, rp->ai_addrlen) == -1)
								{
												log("bind %s", gai_strerror(errno));
												close(sockfd[ret-1]);
												sockfd[ret-1] = -1;
												continue;
								}
								if(listen(sockfd[ret-1], SOMAXCONN) == -1)
								{

												log("listen %s", gai_strerror(errno));
												close(sockfd[ret-1]);
												sockfd[ret-1] = -1;
												continue;
								}
								if(set_nonblocking(sockfd[ret-1]) == -1)
								{
												log("set_nonblocking %s", gai_strerror(errno));
												close(sockfd[ret-1]);
												sockfd[ret-1] = -1;
												continue;
								}
								event.data.fd = sockfd[ret-1];
								event.events = EPOLLIN | EPOLLET;
								if(epoll_ctl(efd, EPOLL_CTL_ADD, sockfd[ret-1], &event) == -1)
								{
												log("epoll_ctl %s", gai_strerror(errno));
												close(sockfd[ret-1]);
												sockfd[ret-1] = -1;
												continue;
								}
								if(ret == 2)
								{
												break;
								}
				}
				freeaddrinfo(result);
				if( (sockfd[0] != -1) && (sockfd[1] != -1) )
								return 0;
				return -1;
}

int set_nonblocking(int socketfd)
{
				int flags;
				if( (flags = fcntl(socketfd, F_GETFL, 0)) == -1)
								return -1;
				flags |= O_NONBLOCK;
				if(fcntl(socketfd, F_SETFL, flags))
								return -1;
				return 0;
}

void * accept_thread(void * data)
{
				pthread_t tid = pthread_self();
				pthread_detach(tid);
				int fd = (int) data;
				while(1)
				{
								int infd;
								struct sockaddr inaddr;
								char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
								socklen_t len = sizeof inaddr;
								if( (infd = accept(fd, &inaddr, &len)) == -1)
								{
												if( (errno != EAGAIN) || (errno != EWOULDBLOCK) )
																log("Thread[0x%lX] accept %s\n", tid, strerror(errno));
												break;
								}
								if(getnameinfo(&inaddr, len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
								{
												log("Thread[0x%lX] getnameinfo from FD[%d] %s, closing it\n", tid, infd, strerror(errno));
												close(infd);
												break;
								}
								SSL * ssl = SSL_new(ctx);
								SSL_set_fd(ssl, infd);
								int s;
								if ((s = SSL_accept(ssl)) != 1) 
								{
												log("Thread[0x%lX] SSL_accept FD[%d] %s, closing it\n", tid, infd, strerror(errno));
												close(infd);
												SSL_shutdown(ssl);
												free(ssl);
												break;
								}
								if(set_nonblocking(infd) == -1)
								{
												log("Thread[0x%lX] set nonblocking FD[%d] %s, closing it\n", tid, infd, strerror(errno));
												close(infd);
												SSL_shutdown(ssl);
												free(ssl);
												break;
								}
								struct data * d = (struct data *)malloc(sizeof(struct data));
								if(d == NULL)
								{
												log("Thread[0x%lX] malloc FD[%d] data %s, closing it\n", tid, infd, strerror(errno));
												close(infd);
												SSL_shutdown(ssl);
												free(ssl);
												break;
								}
								d->fd = infd;
								d->ssl = ssl;
								event.data.ptr = d;
								event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLPRI | EPOLLRDHUP;
								if(epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event) == -1)	
								{
												log("Thread[0x%lX] EPOLL_CTL_ADD FD[%d] %s, closing it\n", tid, infd, strerror(errno));
												close(infd);
												SSL_shutdown(ssl);
												free(ssl);
												free(d);
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
								log("Thread[0x%lX] lock %s\n", tid, strerror(ret));
				list_for_each_safe(listtmp, ntmp, &head)
				{
								sctmp = list_entry(listtmp, struct client, list);
								if(sctmp->readed)
												continue;
								struct data * d = (struct data *)sctmp->cevent.data.ptr;
								int commutfd = d->fd;
								SSL * ssl = d->ssl;
								while(1)
								{
												int count;
												char buf[MAXEVENTS];
																int err;
																err = SSL_get_shutdown(ssl);
												count = SSL_read(ssl, buf, strlen(buf));
												if(count < 1)
												{
																err = SSL_get_error(ssl, count); 
																printf("errno = %d err:%s\n", errno, strerror(errno));
																char * errstr = ERR_error_string(err, buf);
																log("Thread[0x%lX] read FD[%d] %s=%s\n", tid, commutfd,  errstr, buf);
																err = SSL_get_shutdown(ssl);
																break;
												}
												printf("read data: %s\n", buf);
												result = get_date_time(tid);
												count = SSL_write(ssl, result, strlen(result));
												if(count < 1)
												{
																log("Thread[0x%lX] read FD[%d] %s\n", tid, commutfd,  "ssl write");
																break;
												}
												/*if( (count = read(commutfd, buf, sizeof buf)) == -1)
												{
																if(EBADF == errno)
																				pthread_exit(0);
																if(EAGAIN != errno)
																				log("Thread[0x%lX] read FD[%d] %s\n", tid, commutfd,  strerror(errno));
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
																log("Thread[0x%lX] write FD[%d] %s\n", tid, commutfd,  strerror(errno));
																pthread_exit(0);
												}*/
								} // end while
								sctmp->readed = 1;
								sctmp = NULL;
				} // end list for each
				if( (ret = pthread_mutex_unlock(&head_mutex)) != 0)
								log("Thread[0x%lX] unlock %s\n", tid, strerror(ret));
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
												log("Thread[0x%lX] lock FD[%d] %s\n",  tid, sctmp->cevent.data.fd, strerror(ret));
												continue;
								}
								listclient = head.next;
								sctmp = list_entry( listclient, struct client, list);
								if(sctmp->readed == 1)
								{
												struct data * d = (struct data *)sctmp->cevent.data.ptr;
												close(d->fd);
												SSL_shutdown(d->ssl);
												free(d->ssl);
												d->ssl = NULL;
												list_del(&sctmp->list);
												free(d);
												d = NULL;
												free(sctmp);
												sctmp = NULL;
								}
								if( (ret = pthread_mutex_unlock(&head_mutex)) != 0)
												log("Thread[0x%lX] lock FD[%d] %s\n", tid, sctmp->cevent.data.fd, strerror(ret));
				}
				return NULL;
}
