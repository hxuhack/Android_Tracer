#include "fiatsocket.h"
#include"fiatlog.h"

int initSocket()
{
    int socketFD;
	char *server="com.cudroid.fiat";
	struct sockaddr_un socketAddr;
	int result;
	int len;

	socketFD = socket(AF_UNIX, SOCK_STREAM, 0);
	LOGD_GENERAL("Socket Create Success!");
	if(-1 == socketFD)
	{
		LOGD_GENERAL("socket fail");
		return -1;
	}

	socketAddr.sun_family = AF_UNIX;
	socketAddr.sun_path[0] = '\0';
	strcpy(socketAddr.sun_path + 1, server);
	len = 1 + strlen(server) + offsetof(struct sockaddr_un, sun_path);
	result = connect(socketFD, (struct sockaddr*) &socketAddr, len);
	if(result == -1)
	{
		LOGD_GENERAL("connect fail!");
		return -1;
	}
	LOGD_GENERAL("Socket Connect Success!");
	return socketFD;
}


int socksend(int socketFD, char* sendData){

	int ret = write(socketFD, sendData, strlen(sendData));
		//LOGD_GENERAL("ret = %d, socketFD = %d, Data = %s", ret, socketFD, sendData);
	if (ret < 0){
		LOGD_GENERAL("socket error");
	}
	return ret;
}

int sendtofiat(char* data){
	//LOGD_GENERAL("%s", sendData);
    if(socketFD < 0){
	    socketFD = initSocket();
	}
	socksend(socketFD, data);
	return 0;
}
