#include "sys.h"

char* GetHhmmssUsec()
{
    char ret[16];

    long tt;
    struct tm * timeinfo;
    struct timeval tv;

    time(&tt);
    timeinfo = localtime (&tt);

    gettimeofday(&tv,NULL);

    snprintf (ret, sizeof(ret), "%d:%d:%d.%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec);

    return ret;
}

double GetCurrentUsec()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec + tv.tv_usec/1000000;
}

double GetCurrentMillisec()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec/1000;
}