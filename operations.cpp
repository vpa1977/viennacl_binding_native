#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <viennacl/vector.hpp>
#include <viennacl/matrix.hpp>
#include <viennacl/matrix_proxy.hpp>
#include <viennacl/linalg/matrix_operations.hpp>
#include <viennacl/linalg/vector_operations.hpp>
#include <viennacl/linalg/norm_2.hpp>
#include <viennacl/fft.hpp>
#include <stdint.h>

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"



#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "jni_viennacl_context.h"
#include "org_moa_opencl_util_Operations.h"


JNIEXPORT void JNICALL Java_org_moa_opencl_util_Operations_dense_1ax
(JNIEnv * env, jobject obj, jobject matrix, jobject vec,  jobject result,jint rows, jint columns)
{
	native_buffer* matrix_buffer = jni_setup::GetNativeImpl<native_buffer>(env, matrix, "org/viennacl/binding/Buffer");
	native_buffer* vec_buffer = jni_setup::GetNativeImpl<native_buffer>(env, vec, "org/viennacl/binding/Buffer");
	native_buffer* result_buffer = jni_setup::GetNativeImpl<native_buffer>(env, result, "org/viennacl/binding/Buffer");
	if (matrix_buffer->m_data != 0)
	{
		viennacl::matrix<NUM_DATA_TYPE> source(matrix_buffer->m_data, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> vec_data(vec_buffer->m_data, columns);
		viennacl::vector<NUM_DATA_TYPE> result(result_buffer->m_data, rows);
		result = viennacl::linalg::prod(source, vec_data);
		
	}
	else if (matrix_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif
		viennacl::matrix<NUM_DATA_TYPE> source((NUM_DATA_TYPE*)matrix_buffer->m_cpu_data, type, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> vec_data((NUM_DATA_TYPE*)vec_buffer->m_cpu_data, type, columns);
		viennacl::vector<NUM_DATA_TYPE> result((NUM_DATA_TYPE*)result_buffer->m_cpu_data, type, rows);
		result = viennacl::linalg::prod(source, vec_data);

	}
	else
		throw std::runtime_error("invalid buffers");



}