#include "main.h"

static char str[64];

char * get_date_time(void)
{
		time_t t = time(NULL);
		snprintf(str, sizeof str, "%.24s", ctime(&t));
		return str;
}

int bind_sock(char * port)
{
		struct addrinfo inaddr;
		struct addrinfo * result, *rp;
		
		memset(&inaddr, 0, sizeof(struct addrinfo));
		inaddr.ai_family = AF_UNSPEC;
		inaddr.ai_socktype = SOCK_STREAM;
		inaddr.ai_flags = AI_PASSIVE;

		int sfd, ret;

		if( (ret = getaddrinfo(NULL, port, &inaddr, &result)) != 0)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(ret));
				return -1;
		}
		syslog(LOG_INFO, "%s %d %s\n", __FILE__, __LINE__, "Get addr info done");
		for(rp = result; rp != NULL; rp = rp->ai_next)
		{
				if( (sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
						continue;
				if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
						break;
				syslog(LOG_ERR, "%s %d Socket and bind init error %s\n", __FILE__, __LINE__, strerror(errno));
				close(sfd);
		}
		if(rp == NULL)
		{
				syslog(LOG_ERR, "%s %d %s\n", __FILE__, __LINE__, strerror(errno));
				return -1;
		}
		freeaddrinfo(result);
		return sfd;
}

int set_nonblocking(int sfd)
{
		int flags;
		if( (flags = fcntl(sfd, F_GETFL, 0)) == -1)
				return -1;
		flags |= O_NONBLOCK;
		if(fcntl(sfd, F_SETFL, flags))
				return -1;
		return 0;
}

