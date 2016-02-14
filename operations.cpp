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
void do_transform(viennacl::matrix<NUM_DATA_TYPE>& source, viennacl::vector<NUM_DATA_TYPE>& test,
	viennacl::vector<NUM_DATA_TYPE>& result, int rows, int columns)
{

}

JNIEXPORT void JNICALL Java_org_moa_opencl_util_Operations_cosineSimilarity
(JNIEnv * env, jobject obj, jobject matrix, jint columns, jint rows, jobject vec, jobject result)
{
	native_buffer* matrix_buffer = jni_setup::GetNativeImpl<native_buffer>(env, matrix, "org/viennacl/binding/Buffer");
	native_buffer* vec_buffer = jni_setup::GetNativeImpl<native_buffer>(env, vec, "org/viennacl/binding/Buffer");
	native_buffer* result_buffer = jni_setup::GetNativeImpl<native_buffer>(env, result, "org/viennacl/binding/Buffer");
	if (matrix_buffer->m_data != 0)
	{
		viennacl::matrix<NUM_DATA_TYPE> source(matrix_buffer->m_data, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> vec_data(vec_buffer->m_data, columns);
		viennacl::vector<NUM_DATA_TYPE> result(result_buffer->m_data, rows);
		do_transform(source, vec_data, result, rows, columns);
	}
	else if (matrix_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif
		viennacl::matrix<NUM_DATA_TYPE> source((NUM_DATA_TYPE*)matrix_buffer->m_cpu_data, type, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> vec_data((NUM_DATA_TYPE*)vec_buffer->m_cpu_data, type, rows);
		viennacl::vector<NUM_DATA_TYPE> result((NUM_DATA_TYPE*)result_buffer->m_cpu_data, type, columns);
		do_transform(source, vec_data, result, rows, columns);

	}
	else
		throw std::runtime_error("invalid buffers");



}