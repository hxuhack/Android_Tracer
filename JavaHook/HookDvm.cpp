#include "HookDvm.h"
#include "UniversalMethodHandler.h"

using android::AndroidRuntime;


HookInfo* InitHookInfo(char* className, char* methodName, char* paramSig, int isStatic ){
    	HookInfo *info = (HookInfo *)malloc(sizeof(HookInfo));
        	info->className = className;
        	info->methodName = methodName;
        	info->paramSig = paramSig;
        	info->isStaticMethod = isStatic == JNI_TRUE;

        return info;
}

static int CalcMethodArgsSize(const char* shorty) {
	int count = 0;

	/* Skip the return type. */
	shorty++;

	for (;;) {
		switch (*(shorty++)) {
		case '\0': {
			return count;
		}
		case 'D':
		case 'J': {
			count += 2;
			break;
		}
		default: {
			count++;
			break;
		}
		}
	}

	return count;
}

static u4 InvokeHints(const char* shorty) {
	const char* sig = shorty;
	int padFlags, jniHints;
	char sigByte;
	int stackOffset, padMask;

	stackOffset = padFlags = 0;
	padMask = 0x00000001;

	/* Skip past the return type */
	sig++;

	while (true) {
		sigByte = *(sig++);

		if (sigByte == '\0')
			break;

		if (sigByte == 'D' || sigByte == 'J') {
			if ((stackOffset & 1) != 0) {
				padFlags |= padMask;
				stackOffset++;
				padMask <<= 1;
			}
			stackOffset += 2;
			padMask <<= 2;
		} else {
			stackOffset++;
			padMask <<= 1;
		}
	}

	jniHints = 0;

	if (stackOffset > DALVIK_JNI_COUNT_SHIFT) {
		/* too big for "fast" version */
		jniHints = DALVIK_JNI_NO_ARG_INFO;
	} else {
		assert((padFlags & (0xffffffff << DALVIK_JNI_COUNT_SHIFT)) == 0);
		stackOffset -= 2;           // r2/r3 holds first two items
		if (stackOffset < 0)
			stackOffset = 0;
		jniHints |= ((stackOffset + 1) / 2) << DALVIK_JNI_COUNT_SHIFT;
		jniHints |= padFlags;
	}
	return jniHints;
}

static int CalcJniArgInfo(const char* shorty) {
	const char* sig = shorty;
	int returnType, jniArgInfo;
	u4 hints;

	/* The first shorty character is the return type. */
	switch (*(sig++)) {
	case 'V':
		returnType = DALVIK_JNI_RETURN_VOID;
		break;
	case 'F':
		returnType = DALVIK_JNI_RETURN_FLOAT;
		break;
	case 'D':
		returnType = DALVIK_JNI_RETURN_DOUBLE;
		break;
	case 'J':
		returnType = DALVIK_JNI_RETURN_S8;
		break;
	case 'Z':
	case 'B':
		returnType = DALVIK_JNI_RETURN_S1;
		break;
	case 'C':
		returnType = DALVIK_JNI_RETURN_U2;
		break;
	case 'S':
		returnType = DALVIK_JNI_RETURN_S2;
		break;
	default:
		returnType = DALVIK_JNI_RETURN_S4;
		break;
	}

	jniArgInfo = returnType << DALVIK_JNI_RETURN_SHIFT;

	hints = InvokeHints(shorty);

	if (hints & DALVIK_JNI_NO_ARG_INFO) {
		jniArgInfo |= DALVIK_JNI_NO_ARG_INFO;
	} else {
		assert((hints & DALVIK_JNI_RETURN_MASK) == 0);
		jniArgInfo |= hints;
	}

	return jniArgInfo;
}

jclass FindJavaClass(JNIEnv *env,const char *className){

	jclass classObj = env->FindClass(className);

/*	if(env->ExceptionCheck() == JNI_TRUE){
		env->ExceptionClear();
	}
*/
	if(classObj == NULL){
	    LOGD_JAVA("[+] env->FindClass(%s) == NULL\n", className);
	    //TODO: When will it happen? Need more test!
	 }

	return (jclass)env->NewGlobalRef(classObj);
}

static ClassObject* FindClassObj(const char *className){
	JNIEnv *env = AndroidRuntime::getJNIEnv();
	assert(env != NULL);

	char *newclassName = dvmDescriptorToName(className);

	jclass jnicls = FindJavaClass(env, newclassName);
	ClassObject *res = jnicls ? static_cast<ClassObject*>(dvmDecodeIndirectRef(dvmThreadSelf(), jnicls)) : NULL;
	env->DeleteGlobalRef(jnicls);
	free(newclassName);
	return res;
}

 ArrayObject* GetMethodArgs(const Method* method, const u4* args){
	const char* desc = &method->shorty[1]; // [0] is the return type.

	/* count args */
	size_t argCount = dexProtoGetParameterCount(&method->prototype);

	static ClassObject* java_lang_object_array = dvmFindSystemClass("[Ljava/lang/Object;");

	/* allocate storage */
	ArrayObject* argArray = dvmAllocArrayByClass(java_lang_object_array, argCount, ALLOC_DEFAULT);
	if (argArray == NULL)
		return NULL;

	Object** objs = (Object**) (void*) argArray->contents;

	/*
	 * Fill in the array.
	 */
	size_t srcIndex = 0;
	size_t dstIndex = 0;
	while (*desc != '\0') {
		char descChar = *(desc++);
		JValue value;

		switch (descChar) {
		case 'Z':
		case 'C':
		case 'F':
		case 'B':
		case 'S':
		case 'I':
			value.i = args[srcIndex++];
			objs[dstIndex] = (Object*) dvmBoxPrimitive(value, dvmFindPrimitiveClass(descChar));
			/* argObjects is tracked, don't need to hold this too */
			dvmReleaseTrackedAlloc(objs[dstIndex], NULL);
			dstIndex++;
			break;
		case 'D':
		case 'J':
			value.j = dvmGetArgLong(args, srcIndex);
			srcIndex += 2;
			objs[dstIndex] = (Object*) dvmBoxPrimitive(value, dvmFindPrimitiveClass(descChar));
			dvmReleaseTrackedAlloc(objs[dstIndex], NULL);
			dstIndex++;
			break;
		case '[':
		case 'L':
			objs[dstIndex++] = (Object*) args[srcIndex++];
			break;
		}
	}

	return argArray;
}

/*
Transfer Sig to Types
*/
ArrayObject* GetParamTypes(const Method* method, const char* paramSig){
	/* count args */
	size_t argCount = dexProtoGetParameterCount(&method->prototype);
	static ClassObject* java_lang_object_array = dvmFindSystemClass("[Ljava/lang/Object;");

	/* allocate storage */
	ArrayObject* argTypes = dvmAllocArrayByClass(java_lang_object_array, argCount, ALLOC_DEFAULT);
	if(argTypes == NULL){
		return NULL;
	}

	Object** argObjects = (Object**) argTypes->contents;
	const char *desc = (const char *)(strchr(paramSig, '(') + 1);

	/*
	 * Fill in the array.
	 */
	size_t desc_index = 0;
	size_t arg_index = 0;
	bool isArray = false;
	char descChar = desc[desc_index];

	while (descChar != ')') {

		switch (descChar) {
		case 'Z':
		case 'C':
		case 'F':
		case 'B':
		case 'S':
		case 'I':
		case 'D':
		case 'J':
			if(!isArray){
				argObjects[arg_index++] = dvmFindPrimitiveClass(descChar);
				isArray = false;
			}else{
				char buf[3] = {0};
				memcpy(buf, desc + desc_index - 1, 2);
				argObjects[arg_index++] = dvmFindSystemClass(buf);
			}

			desc_index++;
			break;

		case '[':
			isArray = true;
			desc_index++;
			break;

		case 'L':
			int s_pos = desc_index, e_pos = desc_index;
			while(desc[++e_pos] != ';');
			s_pos = isArray ? s_pos - 1 : s_pos;
			isArray = false;

			size_t len = e_pos - s_pos + 1;
			char buf[128] = { 0 };
			memcpy((void *)buf, (const void *)(desc + s_pos), len);
			argObjects[arg_index++] = FindClassObj(buf);
			desc_index = e_pos + 1;
			break;
		}

		descChar = desc[desc_index];
	}
	return argTypes;
}

extern int __attribute__ ((visibility ("hidden"))) HookJavaFunc(JNIEnv *env, HookInfo *info) {

	const char* className = info->className;
	const char* methodName = info->methodName;
	const char* paramSig = info->paramSig;
	const bool isStaticMethod = info->isStaticMethod;

	jclass targetClass = FindJavaClass(env, className);

	if (targetClass == NULL) {
		LOGD_JAVA("[-] %s class not found", className);
		return -1;
	}

	jmethodID methodId =
			isStaticMethod ?
					env->GetStaticMethodID(targetClass, methodName, paramSig) :
					env->GetMethodID(targetClass, methodName, paramSig);

	if (methodId == NULL) {
		LOGD_JAVA("[-] %s->%s method not found", className, methodName);
		return -1;
	}

	// backup method
	Method* method = (Method*) methodId;
	if(method->nativeFunc == UniversalMethodHandler){
		LOGD_JAVA("[*] %s->%s method has already been hooked", className, methodName);
		return -1;
	}
	Method* bakMethod = (Method*) malloc(sizeof(Method));
	memcpy(bakMethod, method, sizeof(Method));

	// init info
	info->originalMethod = (void *)bakMethod;
	//dvmGetBoxedReturnType is method in dalvik/vm/reflect/Reflect.cpp
	info->returnType = (void *)dvmGetBoxedReturnType(bakMethod);
	info->params = GetParamTypes(bakMethod, info->paramSig);

	// hook method
	int argsSize = CalcMethodArgsSize(method->shorty);
	if (!dvmIsStaticMethod(method))
		argsSize++;

	SET_METHOD_FLAG(method, ACC_NATIVE);
	method->registersSize = method->insSize = argsSize;
	method->outsSize = 0;
	method->jniArgInfo = CalcJniArgInfo(method->shorty);

	// save info to insns
	method->insns = (u2*)info;

	// bind the bridge func，only one line
	method->nativeFunc = UniversalMethodHandler;
	LOGD_JAVA("[+] hook %s->%s successful\n", className, methodName);

	return 0;
}