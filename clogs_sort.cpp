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
#include "clogs/radixsort_varkey.h"
#include "org_moa_opencl_util_CLogsSort.h"

struct clogs_sort
{
	clogs_sort(const cl::Context& ctx, const cl::Device& device)
	{
		clogs::RadixsortVarKeyProblem problem;
		problem.setKeyType(clogs::Type(clogs::TYPE_UINT, 1));
		problem.setValueType(clogs::Type(clogs::TYPE_UINT));
		sort_ = new clogs::RadixsortVarKey(ctx, device, problem);
	}
	virtual ~clogs_sort()
	{
		delete sort_;
	}

	void sort(cl::CommandQueue& queue,  cl::Buffer& order_keys, cl::Buffer& bkeys,
		cl::Buffer& values,
		size_t b_key_size, 
		
		size_t len)
	{
		sort_->enqueue(
			queue,
			order_keys,
			bkeys,
			b_key_size,
			values,
			len);
	}

private:
	clogs::RadixsortVarKey* sort_;
};



#define CLAZZ "org/moa/opencl/util/CLogsSort"


/*
* Class:     org_moa_opencl_util_CLogsSort
* Method:    nativeSort
* Signature: (Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;II)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_CLogsSort_nativeSort
(JNIEnv * env, jobject obj, jobject buf_order, jobject buf_keys, jobject buf_values, jint key_size, jint size)
{
	static jclass container_class = env->GetObjectClass(obj);
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, obj, context_field);

	clogs_sort* sort = jni_setup::GetNativeImpl<clogs_sort>(env, obj, CLAZZ);
	native_buffer* buffer_keys = jni_setup::GetNativeImpl<native_buffer>(env, buf_keys, "org/viennacl/binding/Buffer");
	native_buffer* buffer_order = jni_setup::GetNativeImpl<native_buffer>(env, buf_order, "org/viennacl/binding/Buffer");
	native_buffer* buffer_values = jni_setup::GetNativeImpl<native_buffer>(env, buf_values, "org/viennacl/binding/Buffer");
	cl_command_queue command_queue = ctx->opencl_context().get_queue().handle().get();
	clRetainCommandQueue(command_queue);
	cl::CommandQueue q(command_queue);
	clRetainMemObject(buffer_order->m_data);
	clRetainMemObject(buffer_keys->m_data);
	clRetainMemObject(buffer_values->m_data);
	sort->sort(q, cl::Buffer(buffer_order->m_data), cl::Buffer(buffer_keys->m_data),
		cl::Buffer(buffer_values->m_data), key_size, size);

}

/*
* Class:     org_moa_opencl_util_CLogsSort
* Method:    release
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_CLogsSort_release
(JNIEnv * env, jobject obj)
{
	jni_setup::Release<clogs_sort>(env, obj, CLAZZ);

}

/*
* Class:     org_moa_opencl_util_CLogsSort
* Method:    init
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_CLogsSort_init
(JNIEnv *env, jobject obj)
{
	static jclass container_class = env->GetObjectClass(obj);
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, obj, context_field);
	cl::Context clContext(ctx->opencl_context().handle().get());
	cl::Device clDevice(ctx->opencl_context().current_device().id());
	clogs_sort* sort = new clogs_sort(clContext, clDevice);
	jni_setup::Init<clogs_sort>(sort, env, obj, CLAZZ);
	
}
