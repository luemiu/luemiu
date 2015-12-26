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

#include "luemiu.h"

#define MAXEVENTS 1024

int sfd;//socket fd
int efd;//epoll fd
struct epoll_event event;
struct epoll_event * events;

#include "list.h"
extern struct list_head head;//incomming client list 
extern struct list_head dog;//close dog list

extern pthread_mutex_t dog_mutex;
extern pthread_mutex_t head_mutex;
extern pthread_cond_t dog_cond;

struct client
{
		int readed;
		struct epoll_event cevent;//client epoll event
		struct list_head list;		
};

struct dog
{
		int fd;	
		struct list_head list;		
};
#endif
