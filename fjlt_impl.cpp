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
#include "org_moa_gpu_FloatFJLT.h"
#include "org_moa_gpu_MatrixRandomProjection.h"

static long nextPow2(long v) {
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

void do_transform(viennacl::vector<float>& src, viennacl::vector<float>& pre_fft_temp, viennacl::vector<float>& temp, viennacl::vector<float>& ind)
{
	pre_fft_temp = viennacl::linalg::element_prod(src, ind);
	viennacl::fft(pre_fft_temp, temp);
}


void do_transform(viennacl::vector<NUM_DATA_TYPE>& src, viennacl::vector<NUM_DATA_TYPE>& pre_fft_temp,  viennacl::vector<NUM_DATA_TYPE>& temp, viennacl::vector<NUM_DATA_TYPE>& ind)
{
	pre_fft_temp = viennacl::linalg::element_prod(src, ind);
	viennacl::fft(pre_fft_temp, temp);
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


JNIEXPORT void JNICALL Java_org_moa_gpu_FJLT_native_1batch_1update
(JNIEnv * env, jobject obj, jint k , jint n, jobject source, jint rows,  
			jobject pow2src, 
			jobject transform_temp, 
			jobject subst_buffer, 
			jdouble srhst_constant, 
			jobject dst, 
			jobject indicators)
{
	static jclass container_class = env->GetObjectClass(obj);
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, obj, context_field);


	native_buffer* p2_buffer = jni_setup::GetNativeImpl<native_buffer>(env, pow2src, "org/viennacl/binding/Buffer");
	native_buffer* temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, transform_temp, "org/viennacl/binding/Buffer");
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* indicators_buffer = jni_setup::GetNativeImpl<native_buffer>(env, indicators, "org/viennacl/binding/Buffer");
	native_buffer* subst_buffer_buffer = jni_setup::GetNativeImpl<native_buffer>(env, subst_buffer, "org/viennacl/binding/Buffer");
	native_buffer* dst_buffer = jni_setup::GetNativeImpl<native_buffer>(env, dst, "org/viennacl/binding/Buffer");

	if (source_buffer->m_data != 0)
	{
		long len = nextPow2(n);
		viennacl::vector<NUM_DATA_TYPE> src(source_buffer->m_data, n*rows);
		viennacl::vector<NUM_DATA_TYPE> p2(p2_buffer->m_data, len);
		viennacl::vector<NUM_DATA_TYPE> temp(temp_buffer->m_data, len);
		viennacl::vector<NUM_DATA_TYPE> ind(indicators_buffer->m_data, len);
		viennacl::vector<int> substitutions(subst_buffer_buffer->m_data, k);
		viennacl::vector<NUM_DATA_TYPE> destination(dst_buffer->m_data, k*rows);
		viennacl::kernel& permute = ctx->opencl_context().get_kernel("fjlt", "permute");
		permute.local_work_size(0, 256);
		permute.global_work_size(0, 256* 40);
		for (int i = 0; i < rows; ++i)
		{
			viennacl::copy(src.begin() + i*n, src.begin() + (i + 1)*n, p2.begin());
			p2 = viennacl::linalg::element_prod(p2, ind);
			viennacl::fft(p2, temp);
			permute(k, temp.handle(), substitutions.handle(), (NUM_DATA_TYPE)srhst_constant, p2.handle()).enqueue();
			viennacl::copy(p2.begin(), p2.begin() + k, destination.begin() + i*k);
		}
		
	}
	else if (source_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
		long len = nextPow2(n);
		viennacl::vector<NUM_DATA_TYPE> src((NUM_DATA_TYPE*)source_buffer->m_cpu_data, type,n*rows);
		viennacl::vector<NUM_DATA_TYPE> p2((NUM_DATA_TYPE*)p2_buffer->m_cpu_data,  type, len);
		viennacl::vector<NUM_DATA_TYPE> temp((NUM_DATA_TYPE*)temp_buffer->m_cpu_data, type, len);
		viennacl::vector<NUM_DATA_TYPE> ind((NUM_DATA_TYPE*)indicators_buffer->m_cpu_data, type, len);
		viennacl::vector<int> substitutions((int*)subst_buffer_buffer->m_cpu_data, type, k);
		viennacl::vector<NUM_DATA_TYPE> destination((NUM_DATA_TYPE*)dst_buffer->m_cpu_data, type, k*rows);
		viennacl::kernel& permute = const_cast<viennacl::hsa::context&>(ctx->hsa_context()).get_kernel("fjlt", "permute");
		permute.local_work_size(0, 256);
		permute.global_work_size(0, 256 * 40);
		for (int i = 0; i < rows; ++i)
		{
			viennacl::copy(src.begin() + i*n, src.begin() + (i + 1)*n, p2.begin());
			p2 = viennacl::linalg::element_prod(p2, ind);
			viennacl::fft(p2, temp);
			temp *= sqrt(2);
			permute(k, temp.handle(), substitutions.handle(), srhst_constant, p2.handle()).enqueue();
			viennacl::copy(p2.begin(), p2.begin() + k, destination.begin() + i*k);
		}
#else
		throw std::runtime_error("not supported");
#endif
	}
	else
		throw std::runtime_error("invalid buffers");

}
	



JNIEXPORT void JNICALL Java_org_moa_gpu_FloatFJLT_native_1transform
(JNIEnv * env, jobject transform, jobject source, jobject indicators, jint n, jobject pre_fft_temp, jobject temp_obj)
{
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* indicators_buffer = jni_setup::GetNativeImpl<native_buffer>(env, indicators, "org/viennacl/binding/Buffer");
	native_buffer* temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, temp_obj, "org/viennacl/binding/Buffer");
	native_buffer* pre_fft_temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, pre_fft_temp, "org/viennacl/binding/Buffer");

	if (source_buffer->m_data != 0)
	{
		viennacl::vector<float> src(source_buffer->m_data, n);
		viennacl::vector<float> temp(temp_buffer->m_data, n);
		viennacl::vector<float> pre_fft_temp(pre_fft_temp_buffer->m_data, n);
		viennacl::vector<float> ind(indicators_buffer->m_data, n);
		do_transform(src, pre_fft_temp, temp, ind);
	}
	else if (source_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif

		viennacl::vector<float> src((float*)source_buffer->m_cpu_data, type, n);
		viennacl::vector<float> temp((float*)temp_buffer->m_cpu_data, type, n);
		viennacl::vector<float> pre_fft_temp((float*)pre_fft_temp_buffer->m_cpu_data, type, n);
		viennacl::vector<float> ind((float*)indicators_buffer->m_cpu_data, type, n);
		do_transform(src, pre_fft_temp, temp, ind);
	}
	else
		throw std::runtime_error("invalid buffers");
}


JNIEXPORT void JNICALL Java_org_moa_gpu_FloatFJLT_native_1batch_1update
(JNIEnv * env, jobject obj, jint k, jint n, jobject source, jint rows,
	jobject pow2src,
	jobject transform_temp,
	jobject subst_buffer,
	jdouble srhst_constant,
	jobject dst,
	jobject indicators)
{
	static jclass container_class = env->GetObjectClass(obj);
	static jfieldID context_field = env->GetFieldID(container_class, "m_context", "Lorg/viennacl/binding/Context;");
	viennacl::context* ctx = GetContext(env, obj, context_field);


	native_buffer* p2_buffer = jni_setup::GetNativeImpl<native_buffer>(env, pow2src, "org/viennacl/binding/Buffer");
	native_buffer* temp_buffer = jni_setup::GetNativeImpl<native_buffer>(env, transform_temp, "org/viennacl/binding/Buffer");
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* indicators_buffer = jni_setup::GetNativeImpl<native_buffer>(env, indicators, "org/viennacl/binding/Buffer");
	native_buffer* subst_buffer_buffer = jni_setup::GetNativeImpl<native_buffer>(env, subst_buffer, "org/viennacl/binding/Buffer");
	native_buffer* dst_buffer = jni_setup::GetNativeImpl<native_buffer>(env, dst, "org/viennacl/binding/Buffer");

	if (source_buffer->m_data != 0)
	{
		long len = nextPow2(n);
		viennacl::vector<float> src(source_buffer->m_data, n*rows);
		viennacl::vector<float> p2(p2_buffer->m_data, len);
		viennacl::vector<float> temp(temp_buffer->m_data, len);
		viennacl::vector<float> ind(indicators_buffer->m_data, len);
		viennacl::vector<int> substitutions(subst_buffer_buffer->m_data, k);
		viennacl::vector<float> destination(dst_buffer->m_data, k*rows);
		viennacl::kernel& permute = ctx->opencl_context().get_kernel("fjlt", "permute");
		permute.local_work_size(0, 256);
		permute.global_work_size(0, 256 * 40);
		for (int i = 0; i < rows; ++i)
		{
			viennacl::copy(src.begin() + i*n, src.begin() + (i + 1)*n, p2.begin());
			p2 = viennacl::linalg::element_prod(p2, ind);
			viennacl::fft(p2, temp);
			permute(k, temp.handle(), substitutions.handle(), (float)srhst_constant, p2.handle()).enqueue();
			viennacl::copy(p2.begin(), p2.begin() + k, destination.begin() + i*k);
		}

	}
	else if (source_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
		long len = nextPow2(n);
		viennacl::vector<float> src((float*)source_buffer->m_cpu_data, type, n*rows);
		viennacl::vector<float> p2((float*)p2_buffer->m_cpu_data, type, len);
		viennacl::vector<float> temp((float*)temp_buffer->m_cpu_data, type, len);
		viennacl::vector<float> ind((float*)indicators_buffer->m_cpu_data, type, len);
		viennacl::vector<int> substitutions((int*)subst_buffer_buffer->m_cpu_data, type, k);
		viennacl::vector<float> destination((float*)dst_buffer->m_cpu_data, type, k*rows);
		viennacl::kernel& permute = const_cast<viennacl::hsa::context&>(ctx->hsa_context()).get_kernel("fjlt", "permute");
		permute.local_work_size(0, 256);
		permute.global_work_size(0, 256 * 40);
		for (int i = 0; i < rows; ++i)
		{
			viennacl::copy(src.begin() + i*n, src.begin() + (i + 1)*n, p2.begin());
			p2 = viennacl::linalg::element_prod(p2, ind);
			viennacl::fft(p2, temp);
			temp *= sqrt(2);
			permute(k, temp.handle(), substitutions.handle(), srhst_constant, p2.handle()).enqueue();
			viennacl::copy(p2.begin(), p2.begin() + k, destination.begin() + i*k);
		}
#else
		throw std::runtime_error("not supported");
#endif
	}
	else
		throw std::runtime_error("invalid buffers");

}



/*
* Class:     org_moa_gpu_MatrixRandomProjection
* Method:    project
* Signature: (Lorg/viennacl/binding/Buffer;ILorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;)V
*/
JNIEXPORT void JNICALL Java_org_moa_gpu_MatrixRandomProjection_project__Lorg_viennacl_binding_Buffer_2ILorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2II
(JNIEnv * env, jobject obj, jobject output, jint rows, jobject source, jobject projection, jint k, jint n)
{
	native_buffer* projection_buffer = jni_setup::GetNativeImpl<native_buffer>(env, projection, "org/viennacl/binding/Buffer");
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* output_buffer = jni_setup::GetNativeImpl<native_buffer>(env, output, "org/viennacl/binding/Buffer");
#ifdef DEBUG
	int output_size = GetByteSize(env,output);
	int proj_size = GetByteSize(env,projection);
	int input_size = GetByteSize(env,source);
	if (output_size < k*rows * sizeof(float))
		throw std::runtime_error("output buffer is too small");

	if (input_size != rows*n*sizeof(float))
		throw std::runtime_error("invalid input buffer");
	if (proj_size != k*n*sizeof(float))
		throw std::runtime_error("invalid proj buffer");
#endif
	if (projection_buffer->m_data != 0)
	{
		viennacl::matrix<float> proj(projection_buffer->m_data, k, n);
		viennacl::matrix<float, viennacl::column_major> src(source_buffer->m_data, n, rows);
		viennacl::matrix<float, viennacl::column_major> result(output_buffer->m_data, k, rows);
		result = viennacl::linalg::prod(proj, src);

	}
	else if (projection_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif
		viennacl::matrix<float> proj((float*)projection_buffer->m_cpu_data,type, k, n);
		viennacl::matrix<float, viennacl::column_major> src((float*)source_buffer->m_cpu_data, type, n, rows);
		viennacl::matrix<float, viennacl::column_major> result((float*)output_buffer->m_cpu_data, type, k, rows);

		result = viennacl::linalg::prod(proj, src);

	}
	else
		throw std::runtime_error("invalid buffers");


}

/*
* Class:     org_moa_gpu_MatrixRandomProjection
* Method:    project
* Signature: (Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;Lorg/viennacl/binding/Buffer;)V
*/
JNIEXPORT void JNICALL Java_org_moa_gpu_MatrixRandomProjection_project__Lorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2Lorg_viennacl_binding_Buffer_2II
(JNIEnv * env, jobject obj, jobject output, jobject source, jobject projection, jint k, jint n)
{
	native_buffer* projection_buffer = jni_setup::GetNativeImpl<native_buffer>(env, projection, "org/viennacl/binding/Buffer");
	native_buffer* source_buffer = jni_setup::GetNativeImpl<native_buffer>(env, source, "org/viennacl/binding/Buffer");
	native_buffer* output_buffer = jni_setup::GetNativeImpl<native_buffer>(env, output, "org/viennacl/binding/Buffer");
	if (projection_buffer->m_data != 0)
	{
		viennacl::matrix<float> proj(projection_buffer->m_data, k, n);
		viennacl::vector<float> src(source_buffer->m_data, n);
		viennacl::vector<float> result(output_buffer->m_data,k);
		result = viennacl::linalg::prod(proj, src);

	}
	else if (projection_buffer->m_cpu_data != 0)
	{
		viennacl::memory_types type = viennacl::MAIN_MEMORY;
#ifdef VIENNACL_WITH_HSA
		type = viennacl::HSA_MEMORY;
#endif
		viennacl::matrix<float> proj((float*)projection_buffer->m_cpu_data, type, k, n);
		viennacl::vector<float> src((float*)source_buffer->m_cpu_data, type, n);
		viennacl::vector<float> result((float*)output_buffer->m_cpu_data, type, k);
		result = viennacl::linalg::prod(proj, src);

	}
	else
		throw std::runtime_error("invalid buffers");
}

