#ifndef __UTILES_SYS_H__
#define __UTILES_SYS_H__

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

//#ifndef __TIME_T
//#define __TIME_T     /* 避免重复定义 time_t */
typedef long time_t;    /* 时间值time_t 为长整型的别名*/
//#endif

//long GetCurrentMillisec();
char* GetHhmmssUsec();

#endif