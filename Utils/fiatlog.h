#ifndef __FIATLOG_H__
#define __FIATLOG_H__


#include <android/log.h>

#define LOGD_C(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, "PAYLOAD_C", fmt, ##args)
#define LOGD_JAVA(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, "PAYLOAD_JAVA", fmt, ##args)
#define LOGD_GENERAL(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, "PAYLOAD_GENERAL", fmt, ##args)

#endif