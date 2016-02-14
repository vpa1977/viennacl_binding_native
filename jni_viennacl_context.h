#ifndef JNI_VIENNACL_CONTEXT_HPP
#define JNI_VIENNACL_CONTEXT_HPP
#include <jni.h>
#include "jni_setup.h"
#include <viennacl/context.hpp>

#define JAVA_BIND_MAIN_MEMORY 0
#define JAVA_BIND_OPENCL_MEMORY 1
#define JAVA_BIND_HSA_MEMORY 2 

struct native_buffer
{
	native_buffer()
	{
		m_cpu_data = 0;
#ifdef VIENNACL_WITH_OPENCL
		m_data = 0;
		m_data_host_ptr = 0;
#endif
	}
	virtual ~native_buffer()
	{
#ifdef VIENNACL_WITH_OPENCL
		if (m_data)
		{
			free(m_data_host_ptr);
			clReleaseMemObject(m_data);
			m_cpu_data = 0;
			m_data_host_ptr = 0;
			m_data = 0;
		}
#endif
		if (m_cpu_data)
		{
			if (m_data)
				throw std::runtime_error("Memory Corruption");
			delete m_cpu_data;
			m_cpu_data = 0;
		}

	}

	void *m_cpu_data;
#ifdef VIENNACL_WITH_OPENCL
	cl_mem m_data;
	void *m_data_host_ptr;
#endif
};

jlong GetByteSize(JNIEnv * env, jobject obj);
viennacl::context* GetContext(JNIEnv* env, jobject obj, jfieldID context_field);

#endif
