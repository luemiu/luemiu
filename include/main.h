#ifndef __MAIN_H__
#define __MAIN_H__
#include <stdio.h>  
#include <sys/types.h>
#include <stdlib.h>  
#include <unistd.h>  
#include <errno.h>  
#include <err.h>
#include <sys/socket.h>  
#include <time.h>
#include <netdb.h>  
#include <fcntl.h>  
#include <sys/epoll.h>  
#include <string.h>  
#include <pthread.h>
#include <syslog.h>
#include <assert.h>
#include <execinfo.h>
#include <signal.h>

#define APPNAME "luemiu"

#include "../include/miu.h"
#include "../include/dump.h"

#define FREE(p)  do {                                                                                                   \
		                	syslog(LOG_INFO, "%s %s %d free(0x%lX)\n", __FILE__, __func__, __LINE__, (unsigned long)p); \
		                	free(p);                                                                                    \
		        } while (0)

#define MALLOC(s) Malloc(s)

#define MAXEVENTS 128

int sfd;//socket fd
int efd;//epoll fd
struct epoll_event event;
struct epoll_event * events;

#include "list.h"
extern struct list_head head;//incomming client list 
extern struct list_head dog;//close dog list

extern pthread_mutex_t head_mutex;

struct client
{
		int readed;
		struct epoll_event cevent;//client epoll event
		struct list_head list;		
};

#endif
