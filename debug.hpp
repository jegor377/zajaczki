#ifdef DEBUG

#include <cstdio>

#define debug(FORMAT,...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(proc_rank/7))%2, 31+(6+proc_rank)%7, proc_rank, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif