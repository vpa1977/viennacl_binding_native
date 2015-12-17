#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <viennacl/context.hpp>
#include <viennacl/vector.hpp>

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include "merge_sort.h"


#include "jni_setup.h"
#include "jni_viennacl_context.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "org_viennacl_binding_Kernel.h"
#include "org_moa_opencl_util_DoubleMergeSort.h"



#define DOUBLE_MERGE_SORT "org/moa/opencl/util/DoubleMergeSort"


/*
* Class:     org_moa_opencl_util_DoubleMergeSort
* Method:    init
* Signature: (Lorg/viennacl/binding/Context;)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_DoubleMergeSort_init
(JNIEnv * env, jobject obj, jobject context)
{
	merge_sorter<double>* sorter;
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, context,"org/viennacl/binding/Context" );
	sorter = new merge_sorter<double>(*ctx);
	jni_setup::Init<merge_sorter<double> >(sorter, env, obj, DOUBLE_MERGE_SORT);
}

/*
* Class:     org_moa_opencl_util_DoubleMergeSort
* Method:    release
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_DoubleMergeSort_release
(JNIEnv * env, jobject obj)
{
	jni_setup::Release<merge_sorter<double> >(env, obj,DOUBLE_MERGE_SORT);
}
/*
* Class:     org_moa_opencl_util_DoubleMergeSort
* Method:    nativeSort
* Signature: (Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_util_DoubleMergeSort_nativeSort
(JNIEnv * env, jobject obj, jobject input, jobject tmp, jobject src, jobject dst)
{
	merge_sorter<double>* sorter = jni_setup::GetNativeImpl< merge_sorter<double> >(env, obj, DOUBLE_MERGE_SORT);
	native_buffer* input_buffer = jni_setup::GetNativeImpl<native_buffer>(env, input, "org/viennacl/binding/Buffer");
	native_buffer* tmp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, tmp, "org/viennacl/binding/Buffer");
	native_buffer* src_buffer = jni_setup::GetNativeImpl<native_buffer>(env, src, "org/viennacl/binding/Buffer");
	native_buffer* dst_buffer = jni_setup::GetNativeImpl<native_buffer>(env, dst, "org/viennacl/binding/Buffer");
	jlong size = GetByteSize(env, input) / sizeof(double);

	switch (sorter->context().memory_type())
	{
	case viennacl::OPENCL_MEMORY:
	{
		viennacl::vector<double> input_proxy(input_buffer->m_data, size);
		viennacl::vector<double> tmp_proxy(tmp_buffer->m_data, size);
		viennacl::vector<unsigned int> src_proxy(src_buffer->m_data, size);
		viennacl::vector<unsigned int> dst_proxy(dst_buffer->m_data, size);
		sorter->merge_sort(input_proxy, tmp_proxy, src_proxy, dst_proxy);
	}
	break;
	case viennacl::HSA_MEMORY:
	{
		viennacl::vector<double> input_proxy((double*)input_buffer->m_cpu_data, viennacl::HSA_MEMORY, size);
		viennacl::vector<double> tmp_proxy((double*)tmp_buffer->m_cpu_data, viennacl::HSA_MEMORY, size);
		viennacl::vector<unsigned int> src_proxy((unsigned int*)src_buffer->m_cpu_data, viennacl::HSA_MEMORY, size);
		viennacl::vector<unsigned int> dst_proxy((unsigned int*)dst_buffer->m_cpu_data, viennacl::HSA_MEMORY, size);
		sorter->merge_sort(input_proxy, tmp_proxy, src_proxy, dst_proxy);
	}
	break;
	default:
		throw std::runtime_error("Unsupported memory type");
	}
}
