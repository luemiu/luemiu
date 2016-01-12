#include "../include/main.h"

LIST_HEAD(head);
pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;
int main (int argc, char *argv[])  
{ 	
				if( (logfd = fopen("miu.log", "a")) == NULL)
				{
								printf("open log file :%s\n", strerror(errno));
								return EXIT_FAILURE;
				}
				sockfd[0] = -1;
				sockfd[1] = -1;
				int args = optargs(argc, argv);
				if(!(args & cflag))
				{
								log("Need -c option to specifies a public key file\n");
								return EXIT_FAILURE;
				}
				if(!(args & rflag))
				{
								log("Need -r option to specifies a private key file\n");
								return EXIT_FAILURE;
				}
				if(args & dflag)
								daemon(0, 0);
				char port[] = "8008";
				pthread_t ctid;//operation client data thread
				if (signal(SIGSEGV, dump) == SIG_ERR)
								log("Dump SIG_ERR\n");
				SSL_library_init();
				OpenSSL_add_all_algorithms();
				SSL_load_error_strings();
				if( (ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
				{
								ERR_print_errors_fp(stdout);
								return EXIT_FAILURE;
				}
				if (SSL_CTX_use_certificate_file(ctx, argpublic, SSL_FILETYPE_PEM) <= 0) 
				{
								ERR_print_errors_fp(stdout);
								return EXIT_FAILURE;
				}
				if (SSL_CTX_use_PrivateKey_file(ctx, argprivate, SSL_FILETYPE_PEM) <= 0) 
				{
								ERR_print_errors_fp(stdout);
								return EXIT_FAILURE;
				}
				if (!SSL_CTX_check_private_key(ctx)) 
				{
								ERR_print_errors_fp(stdout);
								return EXIT_FAILURE;
				}
				if( (efd = epoll_create1 (0)) == -1)  
				{
								log("epoll_create1 %s\n", strerror(errno));
								return EXIT_FAILURE;
				}
				if(bind_sock(port) == -1)
				{
								close(efd);
								return EXIT_FAILURE;
				}
				if( (events = (struct epoll_event *)MALLOC(MAXEVENTS*sizeof(event))) == NULL)
				{
								log("Can not calloc!");
								close(efd);
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
												if((events[i].data.fd == sockfd[0]) || (events[i].data.fd == sockfd[1]))
												{
																if( (result = pthread_create(&ctid, NULL, accept_thread, (void*)events[i].data.fd)) != 0)
																{
																				log("event[%d] accept thread create %s\n", n, strerror(result));
																}
																continue;
												}
												if( (events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP)
																				|| (events[i].events & EPOLLERR) || (events[i].events & EPOLLPRI) )
												{
																continue;
												}
												if(events[i].events & EPOLLIN)
												{
																if( (result =pthread_mutex_lock(&head_mutex)) != 0)
																				log("pthread_mutex_lock %s\n", strerror(result));
																struct client * sc = (struct client *)malloc(1*sizeof(struct client));
																if(sc == NULL)
																{
																				log("Malloc events[%d]\n", i);
																				close(events[i].data.fd);
																				if( (result = pthread_mutex_unlock(&head_mutex)) != 0)
																								log("pthread_mutex_unlock %s\n",  strerror(result));
																				continue;
																}
																bzero(sc, sizeof(struct client));
																sc->cevent = events[i];
																sc->readed = 0;
																list_add_tail(&sc->list, &head);	
																sc = NULL;
																if( (result = pthread_mutex_unlock(&head_mutex)) != 0)
																				log("pthread_mutex_unlock %s\n",  strerror(result));
																opflag = 1;
												}
								} // end for
								if(opflag)
								{
												if( (result = pthread_create(&ctid, NULL, commut_thread, NULL)) != 0)
												{
																log("commut thread create %s, conntinue\n", strerror(result));
																continue;
												}
								}
								if(freeflag)
								{
												if( (result = pthread_create(&ctid, NULL, free_thread, NULL)) != 0)
												{
																log("Free thread create %s\n", strerror(result));
																freeflag = 1;
												}
												freeflag = 0;
								}
				} // end while 
				free(events);  
				close(sockfd[0]);  
				close(sockfd[1]);  
				close(efd);  
				fclose(logfd);
				return EXIT_SUCCESS;  
}  
