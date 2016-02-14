// viennacl-java-binding.cpp : Defines the exported functions for the DLL application.
//

#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"

#ifndef UNIX
#pragma warning (disable:4297)
#endif


#ifdef VIENNACL_WITH_HSA
#include <hsa.h>

static hsa_region_t HSA_GlobalRegion;

/* Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
   and sets the value of data to the agent handle if it is.
*/
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

/* Find the global fine grained region */
extern hsa_status_t find_global_region(hsa_region_t region, void* data)
{
         if(NULL == data) return HSA_STATUS_ERROR_INVALID_ARGUMENT;

         hsa_status_t err;
         hsa_region_segment_t segment;
         uint32_t flag;

         err = hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);

         err = hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flag);

         if((HSA_REGION_SEGMENT_GLOBAL == segment) && (flag & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED)) {
                 *((hsa_region_t*)data) = region;
         }

         return HSA_STATUS_SUCCESS;
}

extern void free_global(void* free_pointer) {
    hsa_status_t err;
    err = hsa_memory_free(free_pointer);
    return ;
}

extern void*  malloc_global(size_t sz) {
    void* temp_pointer;
    hsa_status_t err;
    err = hsa_memory_allocate(HSA_GlobalRegion, sz, (void**)&temp_pointer);
    return temp_pointer;
}
#endif


static void GetJStringContent(JNIEnv *AEnv, jstring AStr, std::string &ARes) {
	if (!AStr) {
		ARes.clear();
		return;
	}

	const char *s = AEnv->GetStringUTFChars(AStr, NULL);
	ARes = s;
	AEnv->ReleaseStringUTFChars(AStr, s);
}


/*
* Class:     org_viennacl_binding_Context
* Method:    init
* Signature: (I)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_init
(JNIEnv * env, jobject context_wrapper, jint type, jstring params)
{
	viennacl::context* ptr;
	switch (type)
	{
	case 0:
		ptr = new viennacl::context(viennacl::MAIN_MEMORY);
		break;
#ifdef VIENNACL_WITH_OPENCL
	case 1:
	{
		std::string text;
		GetJStringContent(env, params, text);
		ptr = new viennacl::context(viennacl::OPENCL_MEMORY);
		if (text.size() > 0)
			ptr->opencl_context().build_options(text);

		std::vector<viennacl::ocl::device> dev = ptr->opencl_context().devices();
		const char* const possible_device = getenv("OPENCL_DEVICE");
		if (possible_device != NULL)
		{
			std::cout << "Requested device " << possible_device << std::endl;

			for (int i = 0; i < dev.size(); ++i)
			{
				std::cout << "Found device " << dev.at(i).name() << std::endl;
				if (dev.at(i).name() == possible_device)
				{
					ptr->opencl_context().switch_device(dev.at(i));
					break;
				}

			}
		}
		std::cout << "Using device " << ptr->opencl_context().current_device().name() << std::endl; 
	}
		break;
#endif
#ifdef VIENNACL_WITH_HSA
	case 2:
	{
		ptr = new viennacl::context(viennacl::HSA_MEMORY);
		// setup memory allocation
		 /* Iterate over the agents and pick the gpu agent */
		hsa_status_t err;
		hsa_agent_t thisAgent;
		err = hsa_iterate_agents(get_gpu_agent, &thisAgent);
	    /* Find the global region to support malloc_global */
	    hsa_agent_iterate_regions(thisAgent, find_global_region, &HSA_GlobalRegion);
	    err = (HSA_GlobalRegion.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;

	}
		break;
#endif
	default:
		throw std::runtime_error("Unsupported memory type");
	}
	jni_setup::Init<viennacl::context>(ptr, env, context_wrapper, "org/viennacl/binding/Context");

}

/*
* Class:     org_viennacl_binding_Context
* Method:    release
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_release
(JNIEnv * env, jobject obj)
{
	jni_setup::Release<viennacl::context>(env, obj, "org/viennacl/binding/Context");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    addProgram
* Signature: (Ljava/lang/String;Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_addProgram
(JNIEnv * env, jobject obj, jstring jprogram, jstring jtext,  jstring jbuild_options) 
{
	std::string program;
	std::string text;
	std::string build_options;

	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		GetJStringContent(env, jprogram, program);
		GetJStringContent(env, jtext, text);
		GetJStringContent(env, jbuild_options, build_options);
		ctx->opencl_context().build_options(build_options);
		ctx->opencl_context().add_program(text, program);
		ctx->opencl_context().build_options("");
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
#ifdef VIENNACL_WITH_HSA
			GetJStringContent(env, jprogram, program);
			GetJStringContent(env, jtext, text);
			GetJStringContent(env, jbuild_options, build_options);
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).build_options(build_options);
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).add_program(text, program);
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).build_options("");
#endif
		}
		else
			throw std::runtime_error("Unsupported context type");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    finishDefaultQueue
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_finishDefaultQueue
(JNIEnv * env, jobject obj)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		ctx->opencl_context().get_queue().finish();
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
#ifdef VIENNACL_WITH_HSA
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).get_queue().finish();
#endif
		}
		else
			throw std::runtime_error("Unsupported context type");
}


/*
* Class:     org_viennacl_binding_Context
* Method:    finishDefaultQueue
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_submitBarrier
(JNIEnv * env, jobject obj, jboolean val)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		// do nothing OPENCL does not need it
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
#ifdef VIENNACL_WITH_HSA
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).get_queue().submit_barrier(val);
#endif
		}
		else
			throw std::runtime_error("Unsupported context type");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    removeProgram
* Signature: (Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_viennacl_binding_Context_removeProgram
(JNIEnv * env, jobject obj, jstring jprogram)
{
	std::string program;

	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		GetJStringContent(env, jprogram, program);
		ctx->opencl_context().delete_program(program);
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
#ifdef VIENNACL_WITH_HSA
			GetJStringContent(env, jprogram, program);
			const_cast<viennacl::hsa::context&>(ctx->hsa_context()).delete_program(program);
#endif
		}
		else
			throw std::runtime_error("Unsupported context type");
}

/*
* Class:     org_viennacl_binding_Context
* Method:    getKernel
* Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/viennacl/binding/Kernel;
*/
JNIEXPORT jobject JNICALL Java_org_viennacl_binding_Context_nativeGetKernel
(JNIEnv * env, jobject obj, jstring jprogram, jstring jkernel, jobject kernel)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	static jclass kernel_class = env->FindClass("org/viennacl/binding/Kernel");
	std::string program;
	std::string kernel_name;
	

	if (ctx->memory_type() == viennacl::OPENCL_MEMORY)
	{
#ifdef VIENNACL_WITH_OPENCL
		GetJStringContent(env, jprogram, program);
		GetJStringContent(env, jkernel, kernel_name);
		try
		{
			viennacl::kernel& kern = ctx->opencl_context().get_kernel(program, kernel_name);
			static jfieldID context_field = env->GetFieldID(kernel_class, "m_context", "Lorg/viennacl/binding/Context;");
			env->SetObjectField(kernel, context_field, obj);
			jni_setup::Init<viennacl::kernel>(&kern, env, kernel, "org/viennacl/binding/Kernel");
			return kernel;
		}
		catch (...)
		{
			return 0;
		}
		
		
#endif
	}
	else
		if (ctx->memory_type() == viennacl::HSA_MEMORY)
		{
#ifdef VIENNACL_WITH_HSA
		std::string program;
		std::string kernel_name;
		GetJStringContent(env, jprogram, program);
		GetJStringContent(env, jkernel, kernel_name);
		try
		{
			viennacl::kernel& kern = const_cast<viennacl::hsa::context&>(ctx->hsa_context()).get_kernel(program, kernel_name);
			static jfieldID context_field = env->GetFieldID(kernel_class, "m_context", "Lorg/viennacl/binding/Context;");
			env->SetObjectField(kernel, context_field, obj);
			jni_setup::Init<viennacl::kernel>(&kern, env, kernel, "org/viennacl/binding/Kernel");
			return kernel;
		}
		catch (...)
		{
			return 0;
		}


#endif
		}
		else
			throw std::runtime_error("Unsupported context type");
	return 0;
}


JNIEXPORT jobject JNICALL Java_org_viennacl_binding_Context_createQueue
(JNIEnv * env, jobject obj)
{
	viennacl::context* ctx = jni_setup::GetNativeImpl<viennacl::context>(env, obj, "org/viennacl/binding/Context");
	static jclass queue_class = env->FindClass("org/viennacl/binding/Queue");
	static jmethodID methodID = env->GetMethodID(queue_class, "<init>", "()V");
	jobject queue = env->NewObject(queue_class, methodID);
	int new_queue = ctx->opencl_context().add_queue(ctx->opencl_context().current_device());
	cl_command_queue command_queue = ctx->opencl_context().get_queue(ctx->opencl_context().current_device().id(), new_queue).handle().get();
	jni_setup::Init<_cl_command_queue>(command_queue, env, queue, "org/viennacl/binding/Kernel");
	return queue;
}



