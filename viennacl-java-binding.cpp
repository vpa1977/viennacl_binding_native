// viennacl-java-binding.cpp : Defines the exported functions for the DLL application.
//

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"

#pragma warning (disable:4297)

static void GetJStringContent(JNIEnv *AEnv, jstring AStr, std::string &ARes) {
	if (!AStr) {
		ARes.clear();
		return;
	}

	const char *s = AEnv->GetStringUTFChars(AStr, NULL);
	ARes = s;
	AEnv->ReleaseStringUTFChars(AStr, s);
}


/*
* Class:     org_viennacl_binding_Context
* Method:    init
* Signature: (I)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_init
(JNIEnv * env, jobject context_wrapper, jint type, jstring params)
{
	viennacl::context* ptr;
	switch (type)
	{
	case 0:
		ptr = new viennacl::context(viennacl::MAIN_MEMORY);
		break;
#ifdef VIENNACL_WITH_OPENCL
	case 1:
	{
		std::string text;
		GetJStringContent(env, params, text);
		ptr = new viennacl::context(viennacl::OPENCL_MEMORY);
		if (text.size() > 0)
			ptr->opencl_context().build_options(text);
	}
		break;
#endif
#ifdef VIENNACL_WITH_HSA
	case 2:
		ptr = new viennacl::context(viennacl::HSA_MEMORY);
		break;
#endif
	default:
		throw std::runtime_error("Unsupported memory type");
	}
	jni_setup::Init<viennacl::context>(ptr, env, context_wrapper, "org/viennacl/binding/Context");

}

/*
* Class:     org_viennacl_binding_Context
* Method:    release
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_release
(JNIEnv * env, jobject obj)
{
	jni_setup::Release<viennacl::context>(env, obj, "org/viennacl/binding/Context");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    addProgram
* Signature: (Ljava/lang/String;Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_addProgram
(JNIEnv * env, jobject obj, jstring jprogram, jstring jtext)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		std::string program;
		std::string text;
		GetJStringContent(env, jprogram, program);
		GetJStringContent(env, jtext, text);
		ctx->opencl_context().add_program(text, program);
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
			throw std::runtime_error("Unsupported context type");
		}
		else
			throw std::runtime_error("Unsupported context type");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    removeProgram
* Signature: (Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_removeProgram
(JNIEnv * env, jobject obj, jstring jprogram)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		std::string program;
		GetJStringContent(env, jprogram, program);
		ctx->opencl_context().delete_program(program);
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
			throw std::runtime_error("Unsupported context type");
		}
		else
			throw std::runtime_error("Unsupported context type");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    getKernel
* Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/viennacl/binding/Kernel;
*/
JNIEXPORT jobject JNICALL Java_org_viennacl_binding_Context_nativeGetKernel
(JNIEnv * env, jobject obj, jstring jprogram, jstring jkernel, jobject kernel)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	static jclass kernel_class = env->FindClass("org/viennacl/binding/Kernel");
	

	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		std::string program;
		std::string kernel_name;
		GetJStringContent(env, jprogram, program);
		GetJStringContent(env, jkernel, kernel_name);
		try
		{
			viennacl::kernel& kern = ctx->opencl_context().get_kernel(program, kernel_name);
			static jfieldID context_field = env->GetFieldID(kernel_class, "m_context", "Lorg/viennacl/binding/Context;");
			env->SetObjectField(kernel, context_field, obj);
			jni_setup::Init<viennacl::kernel>(&kern, env, kernel, "org/viennacl/binding/Kernel");
			return kernel;
		}
		catch (...)
		{
			return 0;
		}
		
		
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
			throw std::runtime_error("Unsupported context type");
		}
		else
			throw std::runtime_error("Unsupported context type");
	return 0;
}


JNIEXPORT jobject JNICALL Java_org_viennacl_binding_Context_createQueue
(JNIEnv * env, jobject obj)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	static jclass queue_class = env->FindClass("org/viennacl/binding/Queue");
	static jmethodID methodID = env->GetMethodID(queue_class, "<init>", "()V");
	jobject queue = env->NewObject(queue_class, methodID);
	int new_queue = ctx->opencl_context().add_queue(ctx->opencl_context().current_device());
	cl_command_queue command_queue = ctx->opencl_context().get_queue(ctx->opencl_context().current_device().id(), new_queue).handle().get();
	jni_setup::Init<_cl_command_queue>(command_queue, env, queue, "org/viennacl/binding/Kernel");
	return queue;
}

