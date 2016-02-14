#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <viennacl/vector.hpp>
#include <viennacl/linalg/vector_operations.hpp>
#include <viennacl/fft.hpp>
#include <stdint.h>

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"



#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "jni_viennacl_context.h"
#include "org_moa_gpu_FJLT.h"



void do_transform(viennacl::vector<NUM_DATA_TYPE>& src, viennacl::vector<NUM_DATA_TYPE>& pre_fft_temp,  viennacl::vector<NUM_DATA_TYPE>& temp, viennacl::vector<NUM_DATA_TYPE>& ind)
{

	pre_fft_temp = viennacl::linalg::element_prod(src, ind);
	viennacl::fft(pre_fft_temp, temp);
	temp = sqrt(2) * temp;

}


JNIEXPORT void JNICALL Java_org_moa_gpu_FJLT_native_1transform
(JNIEnv * env, jobject transform, jobject source,  jobject indicators,  jint n, jobject pre_fft_temp, jobject temp_obj)
{
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* indicators_buffer = jni_setup::GetNativeImpl<native_buffer>(env, indicators, "org/viennacl/binding/Buffer");
	native_buffer* temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, temp_obj, "org/viennacl/binding/Buffer");
	native_buffer* pre_fft_temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, pre_fft_temp, "org/viennacl/binding/Buffer");
	
	if (source_buffer->m_data != 0)
	{
		viennacl::vector<NUM_DATA_TYPE> src(source_buffer->m_data, n);
		viennacl::vector<NUM_DATA_TYPE> temp(temp_buffer->m_data, n);
		viennacl::vector<NUM_DATA_TYPE> pre_fft_temp(pre_fft_temp_buffer->m_data, n);
		viennacl::vector<NUM_DATA_TYPE> ind(indicators_buffer->m_data, n);
		do_transform(src, pre_fft_temp, temp, ind);
	}
	else if (source_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif
		
		viennacl::vector<NUM_DATA_TYPE> src((NUM_DATA_TYPE*)source_buffer->m_cpu_data, type, n);
		viennacl::vector<NUM_DATA_TYPE> temp((NUM_DATA_TYPE*)temp_buffer->m_cpu_data, type, n);
		viennacl::vector<NUM_DATA_TYPE> pre_fft_temp((NUM_DATA_TYPE*)pre_fft_temp_buffer->m_cpu_data, type, n);
		viennacl::vector<NUM_DATA_TYPE> ind((NUM_DATA_TYPE*)indicators_buffer->m_cpu_data, type, n);
		do_transform(src, pre_fft_temp, temp, ind);
	}
	else
		throw std::runtime_error("invalid buffers");
}



