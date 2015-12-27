#ifndef __LUEMIU_H__
#define __LUEMIU_H__
#include "main.h"

extern int bind_sock(char *);
extern int set_nonblocking(int);
extern char * get_date_time(unsigned long);
extern void * accept_thread(void *);
extern void * commut_thread(void *);
extern void * free_thread(void *);
extern void * Malloc(size_t);

#endif
