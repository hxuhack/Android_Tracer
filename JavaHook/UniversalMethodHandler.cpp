#include "UniversalMethodHandler.h"
#include "android_runtime/AndroidRuntime.h"
#include "HookDvm.h"
#include <typeinfo>
#include <UtfString.h>

using android::AndroidRuntime;
//TODO: print format
void ParseCallInfo(char *classDesc, char *methodName){
	LOGD_JAVA("Call: %s->%s", classDesc, methodName);
}

//TODO: handler difference parameters
void ParseParams(ArrayObject* argList, ArrayObject* params){
    int argListLength;
     if (argList != NULL){
        argListLength = argList->length;
     }
     else {
        argListLength = 0;
        return;
     }
     DataObject** args = (DataObject**)(void*)argList->contents;
     ClassObject** types = (ClassObject**)(void*)params->contents;
}

//TODO: handler different types
void ParseResult(ClassObject * returnType, DataObject* result){
    char sendData[1024];
    jchar temp;
	//DataObject* tmpResult = (DataObject*) malloc (sizeof(DataObject));
	//memcpy(tmpResult,result,sizeof(DataObject));
    switch(returnType->primitiveType){
        case PRIM_VOID:
	     LOGD_JAVA("Result:void");		 
             break;

        case PRIM_BOOLEAN:
             LOGD_JAVA("%Result:boolean: %x,%s\r\n",  *result->instanceData, *result->instanceData == 1?"true":"false");
             break;
        case PRIM_BYTE:
             LOGD_JAVA("%Result:byte: %c\r\n", *result->instanceData);
             break;
        case PRIM_SHORT:
             LOGD_JAVA("%Result:short: %d\r\n", *result->instanceData);
             break;
        case PRIM_CHAR:
             LOGD_JAVA("%Result:char: %c\r\n", *result->instanceData);
             break;
        case PRIM_INT:
             LOGD_JAVA("%Result:int: %d\r\n", *result->instanceData);
             break;
        case PRIM_LONG:
             LOGD_JAVA("%Result:long: %d\r\n", *result->instanceData);
             break;
        case PRIM_FLOAT:
             LOGD_JAVA("%Result:float: %f\r\n", *result->instanceData);
             break;
        case PRIM_DOUBLE:
             LOGD_JAVA("%Result:double: %f\r\n", *result->instanceData);
             break;
        default: 
            {
		//ALOGW("Unknown primitive type '%c'", returnType->primitiveType);
/*
	     	LOGD_JAVA("result->clazz TypeInfo: %s\r\n", typeid(result->clazz).name());
		LOGD_JAVA("descriptor: %s\r\n", result->clazz->descriptor);
		LOGD_JAVA("InstanceData TypeInfo: %s\r\n", typeid(*(result->instanceData)).name());
		LOGD_JAVA("result->clazz->ifieldCount: %d\r\n", result->clazz->ifieldCount);
		LOGD_JAVA("result->clazz->ifieldRefCount: %d\r\n", result->clazz->ifieldRefCount);
		LOGD_JAVA("result->clazz->PrimitiveType: %d\r\n", result->clazz->primitiveType);
		LOGD_JAVA("result->clazz->directMethodCount: %d\r\n", result->clazz->directMethodCount);
		LOGD_JAVA("result->clazz->initThreadId: %d\r\n", result->clazz->initThreadId);
		LOGD_JAVA("result->clazz->status: %d\r\n", result->clazz->status);
		LOGD_JAVA("result->clazz->objectSize: %d\r\n", result->clazz->objectSize);
	*/
		ClassObject* clazObj = result->clazz;
		InstField* instField = clazObj->ifields;
	/*	
		
		int i=0;
		for(i=0; i<result->clazz->ifieldCount; i++){
			LOGD_JAVA("result->clazz->ifields->name: %s\r\n", instField[i].name);
			LOGD_JAVA("result->clazz->ifields->byteOffset: %d\r\n", instField[i].byteOffset);
		}
*/
		//const StringObject* dataPtr = (StringObject*) result->instanceData;
		const StringObject* strObj = (StringObject*) result;
		char* str = dvmCreateCstrFromString(strObj);
		if(str!=NULL)
			LOGD_JAVA("String: %s\r\n", str);
        }
    }
}

void InvokeObjectMethod(const u4* args, char* className, char* methodName, char* paramSig ){

     JNIEnv *env = AndroidRuntime::getJNIEnv();

     Object* object = (Object*)args[0];
     jclass tmpClass = FindJavaClass(env, className);
     jmethodID tmpMethodId = env->GetMethodID(tmpClass, methodName, paramSig);
     Method* tmpMethod = (Method*) tmpMethodId;
     ArrayObject* tmpArgList = GetMethodArgs(tmpMethod, args + 1);

     ClassObject* returnType = (ClassObject *)dvmGetBoxedReturnType(tmpMethod);
     ArrayObject* params = GetParamTypes(tmpMethod, paramSig);

     DataObject* result = (DataObject*) dvmInvokeMethod(object, tmpMethod, tmpArgList,
                                   params, returnType, true);

     ParseResult(returnType, result);
}

jobject GetSubObject(JNIEnv *env, jobject parentObj, const char * objectName){

	jobject obj = 0;

	jclass cls = env->GetObjectClass(parentObj);
	jfieldID field = env->GetFieldID(cls, objectName, "android/widget/EditText");
	obj = env->GetObjectField(parentObj, field);
	return obj;
}

//Get password for Array MotionPro
void GetPassword(JNIEnv *env, jobject parentObj){
	jobject obj = GetSubObject(env, parentObj, "mEdtPassword");

	jobject subobj;
	JniMethodInfo minfo;

	//Search non-static method
	bool isHave = JniHelper::getMethodInfo(minfo,"android/widget/EditText","getText", "()LEditable/Factory"); 
	if (isHave) {
		subobj = minfo.env -> CallObjectMethod(obj,minfo.methodID);
    }


	isHave = JniHelper::getMethodInfo(minfo,"Editable/Factory","toString", "()Ljava/lang/String"); 
	
	jstring result;
	if (isHave) {
		minfo.env->CallObjectMethod(obj,minfo.methodID, result);
    }

	LOGD_JAVA("%GET ARRAY PASSWORD: %s\r\n", result);
}


void UniversalMethodHandler(const u4* args, JValue* pResult, const Method* method, ::Thread* self){

	HookInfo* info = (HookInfo*)method->insns;
	Method* originalMethod = reinterpret_cast<Method*>(info->originalMethod);
	Object* thisObject = !info->isStaticMethod ? (Object*)args[0]: NULL;
	ArrayObject* argList = GetMethodArgs(originalMethod, info->isStaticMethod ? args : args + 1);
    DataObject* result = (DataObject*) dvmInvokeMethod(thisObject, originalMethod, argList, (ArrayObject *)info->params, (ClassObject *)info->returnType, true);

   /* Method* originalMethod = (Method*) args[1];
	ArrayObject* params = (ArrayObject*) args[2];
    ClassObject* returnType = (ClassObject*) args[3];
    Object* thisObject = (Object*) args[4]; // null for static methods
    ArrayObject* argList = (ArrayObject*) args[5];
    DataObject* result = (DataObject*) dvmInvokeMethod(thisObject, originalMethod, argList, params, returnType, true);*/
	pResult->l = result;

    /*
        We have to evaluate the overhead for invoke getId. Otherwise we have to fullfill this func by monitoring on the calls and results.
    */
    //LOGD_JAVA("IsTouchConsumed?\r\n");
    //if(true == IsTouchConsumed(info->methodName, result)){
        //LOGD_JAVA("TouchEvent is consumed\r\n");
        //InvokeObjectMethod(args, "android/view/View", "getId", "()I");
/*
        char sendData[1024];
        snprintf(sendData, sizeof(sendData), "viewId: %d \r\n", *result->instanceData);
        sendtofiat(sendData);*/
     //}

	//ParseCallInfo(info->className, info->methodName);
     //ParseParams(argList, (ArrayObject *)info->params);
	ParseResult((ClassObject *)info->returnType, result);

	
	JNIEnv *env = AndroidRuntime::getJNIEnv();

	//jclass cls = env->FindClass("net/arraynetworks/mobilenow/portal/PinInputDialog");
	//jobject jobj = env->AllocObject(cls); 
	//Get password for Array MotionPro
	//GetPassword(env,jobj);

	 dvmReleaseTrackedAlloc((Object *)argList, self);
}




