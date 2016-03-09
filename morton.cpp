#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <stdint.h>




#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "jni_viennacl_context.h"
#include "org_moa_opencl_util_MortonCode.h"


std::vector<uint8_t> create_morton_code_byte(int total_dims, int dim, int byte)
{
	std::vector<uint8_t> lookup(total_dims);

	for (unsigned int dim_in_code = 0; dim_in_code < 8; ++dim_in_code)
	{
		uint16_t byte_entry = byte & (1 << dim_in_code);
		int total_shift = (dim_in_code * (total_dims - 1) + dim);
		int pos = total_shift >> 3;
		int byte_shift = total_shift - (pos << 3);
		// reverse byte shift
		//byte_shift = 6 - byte_shift;
		byte_entry = byte_entry << byte_shift;
		lookup[pos] |= byte_entry & 0xFF;
		if ((byte_entry & 0xFF00))
			lookup[pos + 1] |= (byte_entry & 0xFF00) >> 8;
	}
	return lookup;
}


JNIEXPORT void JNICALL Java_org_moa_opencl_util_MortonCode_init
(JNIEnv * env, jobject obj, jlong buffer, jint dims)
{
	uint8_t* buf_ptr = (uint8_t*)buffer;
	int offset = 0;
	for (int dim = 0; dim < dims; ++dim)
		for (int byte = 0; byte < 256; ++byte)
		{
			std::vector<uint8_t> data = create_morton_code_byte(dims, dim, byte);
			memcpy(buf_ptr + offset, &data[0], data.size());
			offset += data.size();
		}


}





