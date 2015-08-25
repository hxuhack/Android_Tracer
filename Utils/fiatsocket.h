#ifndef __FIATSOCKET_H__
#define __FIATSOCKET_H__

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>


#ifdef __cplusplus
extern "C"{
#endif


static int socketFD = -1;
//int initSocket();
//int socksend(int socketFD, char* sendData);
int sendtofiat(char* data);

#ifdef __cplusplus
}
#endif

#endif