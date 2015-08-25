#include "JavaPayload.h"
#include "android_runtime/AndroidRuntime.h"
#include "UniversalMethodHandler.h"

using android::AndroidRuntime;

/*
 * entry
 */
int java_hook_entry(){

	LOGD_JAVA("java_hook_entry start...\n");

    JNIEnv *env = AndroidRuntime::getJNIEnv();

	//HookJavaFunc(env, (HookInfo *) InitHookInfo("android/view/View", "getId", "()I", false));
    //HookJavaFunc(env, (HookInfo *) InitHookInfo("android/view/View", "dispatchTouchEvent", "(Landroid/view/MotionEvent;)Z", false));
    //HookJavaFunc(env, (HookInfo *) InitHookInfo("android/view/View", "dispatchKeyEvent", "(Landroid/view/KeyEvent;)Z", false));
	//HookJavaFunc(env, (HookInfo *) InitHookInfo("android/widget/EditText", "getText", "()Landroid/text/Editable;", false));
	//HookJavaFunc(env, (HookInfo *) InitHookInfo("android/text/Editable", "toString", "()Ljava/lang/String;", false));
	//HookJavaFunc(env, (HookInfo *) InitHookInfo("android/widget/TextView", "getText", "()Ljava/lang/CharSequence;", false));
		HookJavaFunc(env, (HookInfo *) InitHookInfo("java/lang/String", "toString", "()Ljava/lang/String;", false));

	//App-specific class is not supported by findClass
	//HookJavaFunc(env, (HookInfo *) InitHookInfo("net/arraynetworks/mobilenow/portal/PinInputDialog", "clickLogin", "(Landroid/view/View;)V", false));

	return 0;
}
