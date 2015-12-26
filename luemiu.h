#ifndef __LUEMIU_H__
#define __LUEMIU_H__

extern int bind_sock(char *);
extern int set_nonblocking(int);
extern char * get_date_time(void);
extern void * accept_thread(void *);
extern void * commut_thread(void *);
extern void * dog_thread(void *);

#endif
