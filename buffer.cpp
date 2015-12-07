#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "jni_viennacl_context.h"

#pragma warning (disable:4297)

cl_map_flags GetMode(jint mode)
{
	switch (mode)
	{
	case 1:
		return CL_MAP_WRITE;
	case 2:
		return CL_MAP_READ;
	case 3:
		return CL_MAP_READ | CL_MAP_WRITE;
	}
	throw std::runtime_error("Invalid map mode");
}

cl_mem_flags GetMemMode(jint mode)
{
	switch (mode)
	{
	case 1:
		return CL_MEM_WRITE_ONLY;
	case 2:
		return CL_MEM_READ_ONLY;
	case 3:
		return CL_MEM_READ_WRITE;
	}
	throw std::runtime_error("Invalid map mode");
}

inline int GetMemType(JNIEnv * env, jobject obj)
{
	static jclass buffer_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID memory_class_field = env->GetFieldID(buffer_class, "m_memory_type", "I");
	jint mem_type = env->GetIntField(obj, memory_class_field);
	return mem_type;
}

inline int GetBufferMode(JNIEnv * env, jobject obj)
{
	static jclass buffer_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID mode_class_field = env->GetFieldID(buffer_class, "m_mode", "I");
	jint mode_type = env->GetIntField(obj, mode_class_field);
	return mode_type;
}


inline void SetCPUMemoryField(JNIEnv * env, jobject obj, void * the_pointer)
{
	static jclass buffer_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID mem_pointer_field = env->GetFieldID(buffer_class, "m_cpu_memory", "J");
	env->SetLongField(obj, mem_pointer_field, (jlong)the_pointer);
}


/*
* Class:     org_viennacl_binding_Buffer
* Method:    native_copy
* Signature: (Lorg/viennacl/binding/Buffer;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_native_1copy
(JNIEnv * env, jobject obj, jobject target)
{
	jlong size = GetByteSize(env, obj);
	native_buffer * src = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
	native_buffer * dst = jni_setup::GetNativeImpl<native_buffer>(env, target, "org/viennacl/binding/Buffer");
	int mem_type = GetMemType(env, obj);
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
		static jfieldID queue_field = env->GetFieldID(container_class, "m_queue", "Lorg/viennacl/binding/Queue;");

		jobject queue_object = env->GetObjectField(obj, queue_field);

		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		struct _cl_command_queue* queue = 0;
		if (queue_object != 0)
		{
			queue = jni_setup::GetNativeImpl<_cl_command_queue>(env, queue_object, "org/viennacl/binding/Queue");
		}
		else
		{
			queue = ctx->opencl_context().get_queue().handle().get();
		}

		if (clEnqueueCopyBuffer(queue, src->m_data, dst->m_data, 0, 0, size, 0, 0, 0) != CL_SUCCESS)
			throw std::runtime_error("Unable to copy buffer");

	}
	else
#endif
	{
		memcpy(dst->m_cpu_data, src->m_cpu_data, size);
	}
}


/*
* Class:     org_viennacl_binding_Buffer
* Method:    map
* Signature: (I)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_map__I
(JNIEnv * env, jobject obj, jint mode)
{
	native_buffer * ptr = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
	int mem_type = GetMemType(env, obj);
	
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		jlong size = GetByteSize(env, obj);
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
		static jfieldID queue_field = env->GetFieldID(container_class, "m_queue", "Lorg/viennacl/binding/Queue;");

		jobject queue_object = env->GetObjectField(obj, queue_field);
		
		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		struct _cl_command_queue* queue = 0;
		if (queue_object != 0)
		{
			queue = jni_setup::GetNativeImpl<_cl_command_queue>(env, queue_object, "org/viennacl/binding/Queue");
		}
		else
		{
			queue = ctx->opencl_context().get_queue().handle().get();
		}

		cl_map_flags flag_mode = GetMode(mode);
		cl_int err;
		ptr->m_cpu_data = clEnqueueMapBuffer(queue, ptr->m_data, TRUE, flag_mode, 0, size, 0, 0, 0, &err);
		if (err == CL_SUCCESS)
			SetCPUMemoryField(env, obj, ptr->m_cpu_data);
		else
			SetCPUMemoryField(env, obj, 0);

	}
	else
#endif
	{
		SetCPUMemoryField(env, obj, ptr->m_cpu_data );
	}


}

/*
* Class:     org_viennacl_binding_Buffer
* Method:    map
* Signature: (IJJ)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_map__IJJ
(JNIEnv * env, jobject obj, jint mode, jlong offset, jlong size)
{
	native_buffer * ptr = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
	int mem_type = GetMemType(env, obj);
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
		static jfieldID queue_field = env->GetFieldID(container_class, "m_queue", "Lorg/viennacl/binding/Queue;");
		jobject queue_object = env->GetObjectField(obj, queue_field);
		

		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		struct _cl_command_queue* queue = 0;
		if (queue_object != 0)
		{
			queue = jni_setup::GetNativeImpl<_cl_command_queue>(env, queue_object, "org/viennacl/binding/Queue");
		}
		else
		{
			queue = ctx->opencl_context().get_queue().handle().get();
		}

		cl_map_flags flag_mode = GetMode(mode);
		cl_int err;
		ptr->m_cpu_data = clEnqueueMapBuffer(queue, ptr->m_data, TRUE, flag_mode, offset, size, 0, 0, 0, &err);
		if (err == CL_SUCCESS)
			SetCPUMemoryField(env, obj, ptr->m_cpu_data);
		else
			SetCPUMemoryField(env, obj, 0);
	}
	else
#endif
	{
		SetCPUMemoryField(env, obj, ((char*)ptr->m_cpu_data)+offset);
	}

}


/*
* Class:     org_viennacl_binding_Buffer
* Method:    commit
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_commit
(JNIEnv * env, jobject obj)
{
	native_buffer * ptr = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
	int mem_type = GetMemType(env, obj);
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");

		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		viennacl::ocl::command_queue& queue = ctx->opencl_context().get_queue();
		clEnqueueUnmapMemObject(queue.handle().get(), ptr->m_data, ptr->m_cpu_data, 0, 0, 0);
		SetCPUMemoryField(env, obj, 0);
	}
	else
#endif
	{
		SetCPUMemoryField(env, obj, 0);
	}


}


/*
* Class:     org_viennacl_binding_Buffer
* Method:    allocate
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_allocate
(JNIEnv * env, jobject obj)
{
	native_buffer* ptr = new native_buffer();
	int mem_type = GetMemType(env, obj);
	jlong size = GetByteSize(env, obj);

	switch (mem_type)
	{
#ifdef VIENNACL_WITH_OPENCL	
	case JAVA_BIND_OPENCL_MEMORY:
	{
		cl_int err;
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");

		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		ptr->m_data_host_ptr = new uint8_t[size];
		ptr->m_data = clCreateBuffer(raw_context, (GetMemMode(GetBufferMode(env,obj))) | CL_MEM_USE_HOST_PTR, size, ptr->m_data_host_ptr, &err);
		if (ptr->m_data == 0)
		{
			delete ptr->m_data_host_ptr;
			throw std::runtime_error("Buffer allocation failed");
		}
			
	}
		break;
#endif
	case JAVA_BIND_MAIN_MEMORY:
	case JAVA_BIND_HSA_MEMORY:
		ptr->m_cpu_data = new uint8_t[size];
		break;
	default:
		throw std::runtime_error("Unsupported memory type");
	}

	jni_setup::Init<native_buffer>(ptr, env, obj, "org/viennacl/binding/Buffer");
	
}

/*
* Class:     org_viennacl_binding_Buffer
* Method:    release
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_release
(JNIEnv * env, jobject obj)
{
	jni_setup::Release<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
}
