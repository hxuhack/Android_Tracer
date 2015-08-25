#ifndef __JAVA_METHOD_HOOK__H__
#define __JAVA_METHOD_HOOK__H__

#include <jni.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "dvm.h"
#include "android_runtime/AndroidRuntime.h"

jclass FindJavaClass(JNIEnv *env,const char *className);
ArrayObject* GetMethodArgs(const Method* method, const u4* args);
ArrayObject* GetParamTypes(const Method* method, const char* paramSig);

int HookJavaFunc(JNIEnv *env, HookInfo *info);

#endif //end of __JAVA_METHOD_HOOK__H__
