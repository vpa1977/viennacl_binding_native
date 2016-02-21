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
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_native_1copy__Lorg_viennacl_binding_Buffer_2
(JNIEnv * env, jobject obj, jobject target)
{
	jlong size = GetByteSize(env, obj);
	Java_org_viennacl_binding_Buffer_native_1copy__Lorg_viennacl_binding_Buffer_2JJ(env, obj, target, 0, size);
}


/*
* Class:     org_viennacl_binding_Buffer
* Method:    native_copy
* Signature: (Lorg/viennacl/binding/Buffer;JJ)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_native_1copy__Lorg_viennacl_binding_Buffer_2JJ
(JNIEnv * env, jobject obj, jobject target, jlong offset, jlong length)
{
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

		cl_int err = clEnqueueCopyBuffer(queue, src->m_data, dst->m_data, offset, 0, length, 0, 0, 0);

		if (err != CL_SUCCESS)
			throw std::runtime_error("Unable to copy buffer");

	}
	else
#endif
	{

		if (src->m_data || dst->m_data)
			throw std::runtime_error("Memory Corruption!");

		memcpy(dst->m_cpu_data, (char*)(src->m_cpu_data)+offset, length);
	}
}

/*
* Class:     org_viennacl_binding_Buffer
* Method:    native_copy_to
* Signature: (Lorg/viennacl/binding/Buffer;J)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_native_1copy_1to
(JNIEnv *env, jobject obj, jobject target, jlong target_offset)
{
	native_buffer * src = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
	native_buffer * dst = jni_setup::GetNativeImpl<native_buffer>(env, target, "org/viennacl/binding/Buffer");
	int mem_type = GetMemType(env, obj);
	jlong length = GetByteSize(env, obj);
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

		cl_int err = clEnqueueCopyBuffer(queue, src->m_data, dst->m_data, 0, target_offset, length, 0, 0, 0);

		if (err != CL_SUCCESS)
			throw std::runtime_error("Unable to copy buffer");

	}
	else
#endif
	{

		if (src->m_data || dst->m_data)
			throw std::runtime_error("Memory Corruption!");

		memcpy((char*)(dst->m_cpu_data)+target_offset, (char*)(src->m_cpu_data), length);
	}
}

JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_fill__B
(JNIEnv * env, jobject obj, jbyte b)
{
	jlong size = GetByteSize(env, obj);
	native_buffer * src = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
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

		if (clEnqueueFillBuffer(queue, src->m_data, &b, 1, 0, size, 0, 0, 0))
			throw std::runtime_error("Unable to copy buffer");

	}
	else 
#endif
	{
		if (src->m_data)
			throw std::runtime_error("Memory Corruption!");

		memset(src->m_cpu_data, b, size);
	}
}

/*
* Class:     org_viennacl_binding_Buffer
* Method:    fill
* Signature: (BJ)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_fill__BJ
(JNIEnv * env, jobject obj, jbyte b , jlong len)
{
	jlong size = len;
	native_buffer * src = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
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

		cl_int err = clEnqueueFillBuffer(queue, src->m_data, &b, 1, 0, size, 0, 0, 0);
		if (err != CL_SUCCESS)
			throw std::runtime_error("Unable to copy buffer");

	}
	else
#endif
	{
		if (src->m_data)
			throw std::runtime_error("Memory Corruption!");

		memset(src->m_cpu_data, b, size);
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
		/*
		if (ptr->mode & CL_MAP_WRITE)
		{
			clWaitForEvents(1, &(ptr->m_event));
			clReleaseEvent(ptr->m_event);
		}
		*/
		cl_map_flags flag_mode = GetMode(mode);
		cl_int err;
		if (flag_mode & CL_MAP_READ)
		{
			err = clEnqueueReadBuffer(queue, ptr->m_data, true, 0, size, ptr->m_data_host_ptr, 0, 0, 0);
		}
		else
			err = CL_SUCCESS;

		ptr->m_cpu_data = ptr->m_data_host_ptr;
		ptr->mode = flag_mode;
		//ptr->m_cpu_data = clEnqueueMapBuffer(queue, ptr->m_data, true, flag_mode, 0, size, 0, 0, 0, &err);
		if (err == CL_SUCCESS)
			SetCPUMemoryField(env, obj, ptr->m_cpu_data);
		else
			SetCPUMemoryField(env, obj, 0);

	}
	else
#endif
	{
		if (ptr->m_data)
			throw std::runtime_error("Memory Corruption!");
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

		/*
		if (ptr->mode & CL_MAP_WRITE)
		{
			clWaitForEvents(1, &(ptr->m_event));
			clReleaseEvent(ptr->m_event);
		}
		*/

		cl_map_flags flag_mode = GetMode(mode);
		cl_int err;
		if (flag_mode & CL_MAP_READ)
		{
			err = clEnqueueReadBuffer(queue, ptr->m_data, true, offset, size, ptr->m_data_host_ptr, 0, 0, 0);
		}
		else
			err = CL_SUCCESS;
		ptr->m_cpu_data = ptr->m_data_host_ptr;
		ptr->mode = flag_mode;
		//ptr->m_cpu_data = clEnqueueMapBuffer(queue, ptr->m_data, true, flag_mode, offset, size, 0, 0, 0, &err);
		if (err == CL_SUCCESS)
			SetCPUMemoryField(env, obj, ptr->m_cpu_data);
		else
			SetCPUMemoryField(env, obj, 0);
	}
	else
#endif
	{
		if (ptr->m_data)
			throw std::runtime_error("Memory Corruption!");
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
	jlong size = GetByteSize(env, obj);
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		static jclass container_class = env->GetObjectClass(obj);
		static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
				jlong size = GetByteSize(env, obj);
		viennacl::context* ctx = GetContext(env, obj, context_field);
		cl_context raw_context = ctx->opencl_context().handle().get();
		viennacl::ocl::command_queue& queue = ctx->opencl_context().get_queue();
		if (ptr->mode | CL_MAP_WRITE)
			clEnqueueWriteBuffer(queue.handle().get(), ptr->m_data, true, 0, size, ptr->m_cpu_data, 0, 0, 0);
		

		SetCPUMemoryField(env, obj, 0);
	} 
	else 
#endif
	{
		if (ptr->m_data)
			throw std::runtime_error("Memory Corruption!");
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
		ptr->m_data_host_ptr = 0;
		ptr->m_data_host_ptr = malloc(size);
		memset(ptr->m_data_host_ptr, 0, size);
		ptr->m_data = clCreateBuffer(raw_context, (GetMemMode(GetBufferMode(env,obj))) /*| CL_MEM_USE_HOST_PTR*/, size, NULL, &err);
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
	{
#ifdef VIENNACL_WITH_HSA
		ptr->m_cpu_data = malloc_global(size);
#else
		ptr->m_cpu_data = malloc(size);
#endif
#ifdef VIENNACL_DEBUG_KERNEL
		memset(ptr->m_cpu_data, 0, size);
#endif
	}
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
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_release__
(JNIEnv * env, jobject obj)
{
	// use normal map/unmap to make sure that all pending kernel operations are done
	Java_org_viennacl_binding_Buffer_map__IJJ(env, obj, 2, 0, 1);
	Java_org_viennacl_binding_Buffer_commit(env, obj);

#ifdef VIENNACL_WITH_HSA
	int mem_type = GetMemType(env, obj);
	if (mem_type == JAVA_BIND_HSA_MEMORY)
	{
		native_buffer * ptr = jni_setup::GetNativeImpl<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
		free_global(ptr->m_cpu_data);
		ptr->m_cpu_data = NULL;
	}
#endif

	jni_setup::Release<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
}

/*
* Class:     org_viennacl_binding_Buffer
* Method:    release
* Signature: (J)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Buffer_release__J
(JNIEnv *env, jobject obj, jlong ctx)
{
	viennacl::context* context = (viennacl::context*)ctx;
	int mem_type = GetMemType(env, obj);
#ifdef VIENNACL_WITH_OPENCL
	if (mem_type == JAVA_BIND_OPENCL_MEMORY)
	{
		context->opencl_context().get_queue().finish();
	}
#endif
	
#ifdef VIENNACL_WITH_HSA
	if (mem_type == JAVA_BIND_HSA_MEMORY)
	{
		const_cast<viennacl::hsa::context&>(context->hsa_context()).get_queue().finish();
	}
#endif
	jni_setup::Release<native_buffer>(env, obj, "org/viennacl/binding/Buffer");
}

