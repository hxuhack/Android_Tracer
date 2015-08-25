LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -lEGL
#LOCAL_ARM_MODE := arm
#LOCAL_MODULE := inject
#LOCAL_SRC_FILES := inject.c
#include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES:="/opt/taintdroid/frameworks/native/include"
LOCAL_C_INCLUDES+="/opt/taintdroid/frameworks/base/include"
LOCAL_C_INCLUDES+="/opt/taintdroid/system/core/include"
LOCAL_C_INCLUDES+="/opt/taintdroid/libnativehelper/include"
LOCAL_C_INCLUDES+="/opt/taintdroid/dalvik/vm/"
LOCAL_C_INCLUDES+="/opt/taintdroid/dalvik/vm/oo/"

LOCAL_MODULE    := payload

LOCAL_LDLIBS	:= -L$(SYSROOT)/usr/lib -llog
LOCAL_LDLIBS += -L"/home/xuhui/Dropbox/Sourcecode/tracer/JavaHook" -landroid_runtime -ldvm

LOCAL_CFLAGS	:= -DDEBUG -O0
LOCAL_CFLAGS += -DHAVE_SYS_UIO_H

CXX =arm-eabi-g++

LOCAL_SRC_FILES := \
	JavaHook/HookDvm.cpp \
	JavaHook/JavaPayload.cpp \
	JavaHook/UniversalMethodHandler.cpp \
	ElfHook/elfhook.c \
	ElfHook/elfpayload.c \
	Utils/fiatsocket.c \
	Utils/sys.c \
	Utils/JniHelper.cpp
include $(BUILD_SHARED_LIBRARY)
