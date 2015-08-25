#include "elfpayload.h"
#include <jni.h>
//#include "../JavaHook/JavaPayload.h"
/*
 * entry
 */
int hook_entry(char * a){

	LOGD_C("Start hooking hook_IBinder\n");
	//hook_func((void*)ioctl, (void**)&real_ioctl, (void*)new_ioctl, LIBBINDER_PATH);

	//hook_func(connect, &real_connect, new_connect, LIBJAVACORE_PATH);
	//hook_func(sendto, &real_sendto,new_sendto, LIBJAVACORE_PATH);

	java_hook_entry();

	return 0;
}
