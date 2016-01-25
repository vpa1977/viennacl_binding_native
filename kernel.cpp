#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <viennacl/context.hpp>
#include <viennacl/vector.hpp>

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"

#include "jni_setup.h"
#include "jni_viennacl_context.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "org_viennacl_binding_Kernel.h"


#define KERNEL_CLASS "org/viennacl/binding/Kernel"

inline int GetMemType(JNIEnv * env, jobject obj)
{
	static jclass buffer_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID memory_class_field = env->GetFieldID(buffer_class, "m_memory_type", "I");
	jint mem_type = env->GetIntField(obj, memory_class_field);
	return mem_type;
}


/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_global_size
* Signature: (II)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1global_1size
(JNIEnv * env, jobject obj,  jint dim, jint size)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	assert(ptr != 0);
	ptr->global_work_size(dim, size);
}

JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1local_1size
(JNIEnv * env, jobject obj, jint dim, jint size)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	assert(ptr != 0);
	ptr->local_work_size(dim, size);
}

/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_arg
* Signature: (ILjava/lang/Object;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1arg__ILorg_viennacl_binding_Buffer_2
(JNIEnv * env, jobject obj, jint pos, jobject param)
{
	
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);

	static jclass container_class = env->GetObjectClass(obj);
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");

	viennacl::context* context = GetContext(env, obj, context_field);
	assert(ptr != 0);
	jlong size = GetByteSize(env, param);
	native_buffer* buffer = jni_setup::GetNativeImpl<native_buffer>(env, param, "org/viennacl/binding/Buffer");
#ifdef VIENNACL_WITH_OPENCL
	if (buffer->m_data)
	{
#ifdef VIENNACL_DEBUG_KERNEL
		if (GetMemType(env, param) != JAVA_BIND_OPENCL_MEMORY)
		{
			throw std::runtime_error("Memory Corruption");
		}
#endif
		viennacl::vector<uint8_t> proxy(buffer->m_data, size);
		ptr->arg(pos, proxy.handle());
	}
	else
#endif
	{
		viennacl::vector<uint8_t> proxy((uint8_t*)buffer->m_cpu_data, context->memory_type(), size);
		ptr->arg(pos, proxy.handle());
	}
		
	

}


/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_arg
* Signature: (II)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1arg__II
(JNIEnv * env, jobject obj, jint pos, jint val)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	ptr->arg(pos, val);

}

/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_arg
* Signature: (IJ)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1arg__IJ
(JNIEnv *env, jobject obj, jint pos, jlong val)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	ptr->arg(pos, val);
}

/*
* Class:     org_viennacl_binding_Kernel
* Method:    invoke
* Signature: ([Ljava/lang/Object;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_invoke___3Ljava_lang_Object_2
(JNIEnv * env, jobject obj, jobjectArray arr)
{
	jlong len = env->GetArrayLength(arr);
	for (jint pos = 0; pos < len; ++pos)
	{
		jobject element = env->GetObjectArrayElement(arr, pos);
		Java_org_viennacl_binding_Kernel_set_1arg__ILorg_viennacl_binding_Buffer_2(env, obj, pos, element);
	}
	throw std::runtime_error("Not implemented");
}


/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_arg
* Signature: (ID)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1arg__ID
(JNIEnv * env, jobject obj, jint pos, jdouble val)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	ptr->arg(pos, val);
}

/*
* Class:     org_viennacl_binding_Kernel
* Method:    set_arg
* Signature: (IF)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_set_1arg__IF
(JNIEnv * env, jobject obj, jint pos, jfloat val)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	ptr->arg(pos, val);
}




/*
* Class:     org_viennacl_binding_Kernel
* Method:    invoke
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Kernel_invoke__
(JNIEnv * env, jobject obj)
{
	viennacl::kernel* ptr = jni_setup::GetNativeImpl<viennacl::kernel>(env, obj, KERNEL_CLASS);
	ptr->enqueue();
	
}
