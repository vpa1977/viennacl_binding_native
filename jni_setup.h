#ifndef JNI_SETUP_HPP
#define JNI_SETUP_HPP

#include <assert.h>

namespace jni_setup
{
	template <typename T>
	T* GetNativeImpl(JNIEnv* env, jobject instance, const char* const the_class)
	{
		static jclass sgd_class = env->FindClass(the_class);
		static jfieldID context_field = env->GetFieldID(sgd_class, "m_native_context", "J");
		assert(context_field != 0);
		T* impl = (T*)env->GetLongField(instance, context_field);
		return impl;
	}

	template <typename T>
	void Release(JNIEnv* env, jobject instance, const char* const the_class)
	{
		T * ctx = GetNativeImpl<T>(env, instance, the_class);
		delete ctx;
		static jclass _class = env->FindClass(the_class);
		static jfieldID context_field = env->GetFieldID(_class, "m_native_context", "J");
		assert(context_field != 0);
		env->SetLongField(instance, context_field, 0);
	}

	template <typename T>
	void Init(T* new_ctx, JNIEnv* env, jobject instance, const char* const the_class)
	{
		T * ctx = GetNativeImpl<T>(env, instance, the_class);
		delete ctx;
		static jclass _class = env->FindClass(the_class);
		static jfieldID context_field = env->GetFieldID(_class, "m_native_context", "J");
		assert(context_field != 0);
		env->SetLongField(instance, context_field, (jlong)new_ctx);
	}


}


#endif

