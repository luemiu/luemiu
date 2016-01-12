#ifndef __MAIN_H__
#define __MAIN_H__
#include <stdio.h>  
#include <sys/types.h>
#include <stdlib.h>  
#include <unistd.h>  
#include <errno.h>  
#include <err.h>
#include <time.h>
#include <sys/socket.h>  
#include <time.h>
#include <netdb.h>  
#include <fcntl.h>  
#include <sys/epoll.h>  
#include <string.h>  
#include <pthread.h>
#include <assert.h>
#include <execinfo.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define APPNAME "luemiu"
FILE * logfd;
char str[128];
char tstr[64];
char logbuf[256];
#define log(...) do { \
				snprintf(str, sizeof(str), __VA_ARGS__); \
				time_t t = time(NULL); \
				char * ts = ctime(&t); \
				strncpy(tstr, ts, strlen(ts) - 1); \
				snprintf(logbuf, sizeof(logbuf), "%s %s %s %d %s", tstr,__FILE__, __func__, __LINE__, str); \
				fprintf(logfd, logbuf); \
				fflush(logfd); } while(0)

#include "../include/miu.h"
#include "../include/dump.h"

#define FREE(p)  do {                                                                                                   \
		                	syslog(LOG_INFO, "%s %s %d free(0x%lX)\n", __FILE__, __func__, __LINE__, (unsigned long)p); \
		                	free(p);                                                                                    \
		        } while (0)

#define MALLOC(s) Malloc(s)

#define MAXEVENTS 128
enum 
{
				cflag = 1,// cacert key
				dflag = 2,//daemon
				hflag = 4,//help
				pflag = 8,//port
				rflag = 16// private key
};

char * argport;
char * argpublic;
char * argprivate;

SSL_CTX * ctx;

int sockfd[2];//socket fd
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

struct data
{
				int fd;
				SSL * ssl;
};
#endif
