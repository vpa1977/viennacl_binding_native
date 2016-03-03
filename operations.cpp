#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <viennacl/vector.hpp>
#include <viennacl/matrix.hpp>
#include <viennacl/matrix_proxy.hpp>
#include <viennacl/compressed_matrix.hpp>
#include <viennacl/linalg/sparse_matrix_operations.hpp>
#include <viennacl/linalg/sum.hpp>

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
#include "org_moa_opencl_sgd_Multinominal.h"


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

void multi_logistic_loss()
{
#define DATATYPE double 
	int full_batch_size = 128;
	int num_batches = 9;
	int attributes = 768;
	int classes = 10;
	// source batch
	viennacl::compressed_matrix<DATATYPE> source(full_batch_size*num_batches, attributes, viennacl::context());
	// weights
	viennacl::matrix<DATATYPE, viennacl::column_major> weights(attributes, classes);
	// result margins
	viennacl::matrix<DATATYPE> margins(full_batch_size*num_batches, classes);
	margins = viennacl::linalg::prod(source, weights);
}


JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_Multinominal_computeDotProducts__Lorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2IIIIILorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2
(JNIEnv * env, jobject obj, 
	jobject column, 
	jobject row_jumper, 
	jobject elements,
	jobject row_blocks, 
	jint row_block_count, 
	jint columns, 
	jint rows,
	jint element_count,
	jint num_classes,
	jobject weights, 
	jobject margins)
{
	static jclass container_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, column, context_field);
//#undef NUM_DATA_TYPE
//#define NUM_DATA_TYPE float
	native_buffer* column_buffer = jni_setup::GetNativeImpl<native_buffer>(env, column, "org/viennacl/binding/Buffer");
	native_buffer* row_jumper_buffer = jni_setup::GetNativeImpl<native_buffer>(env, row_jumper, "org/viennacl/binding/Buffer");
	native_buffer* elements_buffer = jni_setup::GetNativeImpl<native_buffer>(env, elements, "org/viennacl/binding/Buffer");
	native_buffer* row_blocks_buffer = jni_setup::GetNativeImpl<native_buffer>(env, row_blocks, "org/viennacl/binding/Buffer");
	native_buffer* weights_buffer = jni_setup::GetNativeImpl<native_buffer>(env, weights, "org/viennacl/binding/Buffer");
	native_buffer* margins_buffer = jni_setup::GetNativeImpl<native_buffer>(env, margins, "org/viennacl/binding/Buffer");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
		viennacl::compressed_matrix<NUM_DATA_TYPE> instance_info(row_jumper_buffer->m_data,
			column_buffer->m_data,
			elements_buffer->m_data,
			rows,
			columns,
			element_count,
			row_blocks_buffer->m_data,
			row_block_count,
			*ctx);

		viennacl::matrix<NUM_DATA_TYPE, viennacl::column_major> weights_info(weights_buffer->m_data, columns, num_classes);
		viennacl::matrix<NUM_DATA_TYPE> margin_info(margins_buffer->m_data, rows, num_classes);
		margin_info = viennacl::linalg::prod(instance_info, weights_info);

	}
	else if (ctx->memory_type() == viennacl::HSA_MEMORY)
	{
#ifdef VIENNACL_WITH_HSA
		viennacl::compressed_matrix<NUM_DATA_TYPE> instance_info(
			row_jumper_buffer->m_cpu_data,
			column_buffer->m_cpu_data,
			elements_buffer->m_cpu_data,
			rows,
			columns,
			element_count,
			row_blocks_buffer->m_cpu_data,
			row_block_count);

		viennacl::matrix<NUM_DATA_TYPE, viennacl::column_major> weights_info((NUM_DATA_TYPE*)weights_buffer->m_cpu_data,viennacl::HSA_MEMORY, columns, num_classes);
		viennacl::matrix<NUM_DATA_TYPE> margin_info((NUM_DATA_TYPE*)margins_buffer->m_cpu_data, viennacl::HSA_MEMORY, rows, num_classes);
		margin_info = viennacl::linalg::prod(instance_info, weights_info);

#endif
	}

}

JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_Multinominal_computeDotProducts__Lorg_viennacl_binding_Buffer_2IIILorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2
(JNIEnv * env, jobject obj,
	jobject elements,
	jint columns,
	jint rows,
	jint num_classes,
	jobject weights,
	jobject margins)
{
	static jclass container_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, elements, context_field);
	//#undef NUM_DATA_TYPE
	//#define NUM_DATA_TYPE float
	
	native_buffer* elements_buffer = jni_setup::GetNativeImpl<native_buffer>(env, elements, "org/viennacl/binding/Buffer");
	native_buffer* weights_buffer = jni_setup::GetNativeImpl<native_buffer>(env, weights, "org/viennacl/binding/Buffer");
	native_buffer* margins_buffer = jni_setup::GetNativeImpl<native_buffer>(env, margins, "org/viennacl/binding/Buffer");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		viennacl::matrix<NUM_DATA_TYPE> instance_info(elements_buffer->m_data, rows, columns);
		viennacl::matrix<NUM_DATA_TYPE, viennacl::column_major> weights_info(weights_buffer->m_data, columns, num_classes);
		viennacl::matrix<NUM_DATA_TYPE> margin_info(margins_buffer->m_data, rows, num_classes);
		margin_info = viennacl::linalg::prod(instance_info, weights_info);
#endif
	}
	else
	if (ctx->memory_type() == viennacl::HSA_MEMORY)
	{
#ifdef VIENNACL_WITH_HSA
		viennacl::matrix<NUM_DATA_TYPE> instance_info((NUM_DATA_TYPE*)elements_buffer->m_cpu_data, viennacl::HSA_MEMORY, rows, columns);
		viennacl::matrix<NUM_DATA_TYPE, viennacl::column_major> weights_info((NUM_DATA_TYPE*)weights_buffer->m_cpu_data,viennacl::HSA_MEMORY, columns, num_classes);
		viennacl::matrix<NUM_DATA_TYPE> margin_info((NUM_DATA_TYPE*)margins_buffer->m_cpu_data,viennacl::HSA_MEMORY, rows, num_classes);
		margin_info = viennacl::linalg::prod(instance_info, weights_info);
#endif
	}
	else
		throw std::runtime_error("unsupported");

}


JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_Multinominal_computeReduction
(JNIEnv * env, jobject obj,
	jobject elements,
	jint columns,
	jint rows,
	jobject margins)
{
	static jclass container_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, elements, context_field);
	//#undef NUM_DATA_TYPE
	//#define NUM_DATA_TYPE float
	native_buffer* elements_buffer = jni_setup::GetNativeImpl<native_buffer>(env, elements, "org/viennacl/binding/Buffer");
	native_buffer* margins_buffer = jni_setup::GetNativeImpl<native_buffer>(env, margins, "org/viennacl/binding/Buffer");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
		viennacl::matrix<NUM_DATA_TYPE> dense(elements_buffer->m_data, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> margin_info(margins_buffer->m_data, columns);
		margin_info = viennacl::linalg::row_sum(dense);
	}
	else if (ctx->memory_type() == viennacl::HSA_MEMORY)
	{
#ifdef VIENNACL_WITH_HSA
		viennacl::matrix<NUM_DATA_TYPE> dense((NUM_DATA_TYPE*)elements_buffer->m_cpu_data,viennacl::HSA_MEMORY, rows, columns);
		viennacl::vector<NUM_DATA_TYPE> margin_info((NUM_DATA_TYPE*)margins_buffer->m_cpu_data, viennacl::HSA_MEMORY, columns);
		margin_info = viennacl::linalg::row_sum(dense);

#endif
	}
	else
		throw std::runtime_error("unsupported");

}


