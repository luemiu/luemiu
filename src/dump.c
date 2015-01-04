#include "../include/main.h"

void dump(int sig)
{
		void * buffer[MAXEVENTS] = {0};
		size_t size;
		char **strings = NULL;
		size_t i = 0;

		syslog(LOG_ERR, "%s %s %d dump init, catch signal[%d]\n", __FILE__, __func__, __LINE__, sig);
		size = backtrace(buffer, MAXEVENTS);
		strings = backtrace_symbols(buffer, size);
		if (strings == NULL)
		{
				syslog(LOG_ERR, "%s %s %d backtrace_symbols NULL\n", __FILE__, __func__, __LINE__);
				exit(EXIT_FAILURE);
		}

		for (i = 0; i < size; i++)
		{
				syslog(LOG_ERR, "%s %s %d %d %s\n", __FILE__, __func__, __LINE__, i, strings[i]);
		}
		free(strings);
		strings = NULL;
		exit(0);
}
