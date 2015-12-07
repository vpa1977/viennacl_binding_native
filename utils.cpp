#include "jni_viennacl_context.h"

viennacl::context* GetContext(JNIEnv* env, jobject obj, jfieldID context_field)
{
	jobject context_object = env->GetObjectField(obj, context_field);
	assert(context_object != 0);
	return jni_setup::GetNativeImpl<viennacl::context>(env, context_object, "org/viennacl/binding/Context");
}

jlong GetByteSize(JNIEnv * env, jobject obj)
{
	static jclass buffer_class = env->FindClass("org/viennacl/binding/Buffer");
	static jfieldID byte_size_field = env->GetFieldID(buffer_class, "m_byte_size", "J");
	jlong size = env->GetLongField(obj, byte_size_field);
	return size;
}
