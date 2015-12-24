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

#include "luemiu.h"
#include "client.h"

#define MAXEVENTS 64
#define MAXBUFF 512

int efd;
struct epoll_event event;
struct epoll_event * events;
extern pthread_mutex_t accept_mutex;

#include "list.h"
extern struct list_head incomming_head;//incomming client accept to this

struct client_info
{
		int clientfd;
		struct list_head list;		
};


#endif
