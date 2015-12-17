#ifndef MORTON_CODE_HPP
#define MORTON_CODE_HPP

#include <stdint.h>
#include <boost/multiprecision/cpp_int.hpp>
template<int dims = 2>
struct morton_code
{

	morton_code(viennacl::context& ctx)
	{
		init_cpu_lookup_table();
		lookup_table_ = viennacl::vector<uint32_t>(cpu_lookup_table_.size() / 4, ctx);
		std::vector<uint32_t> temp(cpu_lookup_table_.size() / 4);
		memcpy(&temp[0], &cpu_lookup_table_[0], cpu_lookup_table_.size());
		viennacl::copy(temp, lookup_table_);

	}


	viennacl::vector<uint32_t> z_value_opencl(viennacl::vector<uint32_t>& points, size_t point_count)
	{
		viennacl::context& ctx = viennacl::traits::context(points);
		viennacl::ml::opencl::morton_kernel<viennacl::ocl::context>::init(ctx.opencl_context());
		viennacl::vector<uint32_t> result(dims * point_count, ctx);

		viennacl::ocl::kernel& k = ctx.opencl_context().get_kernel(viennacl::ml::opencl::morton_kernel<viennacl::ocl::context>::program_name(), "morton_code");
		k.local_work_size(1, 1);
		k.global_work_size(0, point_count);
		k.global_work_size(1, 4);
		viennacl::ocl::enqueue(
			k(result, lookup_table_, points, (cl_uint)dims, (cl_uint)point_count)
			);
		//ctx.opencl_context().get_queue().finish();
		return result;
	}

	inline std::vector<uint8_t> z_value_cpu(std::vector<uint32_t>& points, size_t point_count)
	{
		unsigned int dimensions = dims;

		unsigned int rightshift = 32;
		int total_len = dims * 32 / 8;

		std::vector<uint8_t> result(total_len * point_count);
		for (int id = 0; id < point_count; ++id)
		{
			int input_offset = id*dims;
			int output_offset = id * total_len;

			for (unsigned int i_depth = 1; i_depth <= 4; ++i_depth)
			{
				int result_offset = 3 * dimensions - (i_depth - 1)*dimensions; // each mask has [dimensions] bytes in it
				for (int pos = result_offset; pos < result_offset + dimensions; ++pos)
				{
					for (unsigned int d = 0; d < dimensions; ++d)
					{
						int byte = (points[input_offset + d] >> (rightshift - 8 * i_depth)) & 0xFF;
						int offset = d* dims * BYTE_COUNT + byte * dims + pos - result_offset;
						result[output_offset + pos] |= cpu_lookup_table_[offset];
					}
				}
			}
		}
		return result;
	}


	inline std::vector<uint8_t> z_value_cpu(std::vector<uint32_t>& points)
	{
		unsigned int dimensions = dims;

		unsigned int rightshift = 32;
		int total_len = dims * 32 / 8;
		std::vector<uint8_t> result(total_len);

		for (unsigned int i_depth = 1; i_depth <= 4; ++i_depth)
		{
			int result_offset = 3 * dimensions - (i_depth - 1)*dimensions; // each mask has [dimensions] bytes in it
			for (int pos = result_offset; pos < result_offset + dimensions; ++pos)
			{
				for (unsigned int d = 0; d < dimensions; ++d)
				{
					int byte = (points[d] >> (rightshift - 8 * i_depth)) & 0xFF;
					int offset = d* dims * BYTE_COUNT + byte * dims + pos - result_offset;
					result[pos] |= cpu_lookup_table_[offset];
					std::cout << i_depth << " dd " << pos << std::endl;
				}
			}
		}

		return result;
	}

private:
	typedef boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
		32 * dims, 32 * dims,
		boost::multiprecision::unsigned_magnitude,
		boost::multiprecision::unchecked, void> > morton_code_type;
	const int BYTE_COUNT = 256;
	morton_code_type ** generate_lookup_table()
	{
		morton_code_type** lookup_table;
		morton_code_type entry = 0;
		morton_code_type one = 1;

		lookup_table = new morton_code_type*[dims];
		for (unsigned int i = 0; i < dims; ++i)
			lookup_table[i] = new morton_code_type[BYTE_COUNT];

		for (unsigned int dim = 0; dim < dims; ++dim)
		{
			for (unsigned int byte = 0; byte < BYTE_COUNT; ++byte)
			{
				entry = 0;
				// for every dimension participating in this code
				// 8 - max dimension for 0xFF & 2^7
				for (unsigned int dim_in_code = 0; dim_in_code < 8; ++dim_in_code)
				{
					entry |= ((byte & (one << dim_in_code)) << (dim_in_code * (dims - 1) + dim));
				}
				lookup_table[dim][byte] = entry;
			}
		}
		return lookup_table;
	}

	void init_cpu_lookup_table()
	{
		morton_code_type** lookup = generate_lookup_table();

		cpu_lookup_table_.resize(dims* BYTE_COUNT*dims);
		for (unsigned int dim = 0; dim < dims; ++dim)
		{
			for (unsigned int byte = 0; byte < BYTE_COUNT; ++byte)
			{
				// ugly way
				int suggested_len = dims;
				int byte_len = sizeof(lookup[dim][byte].backend().limbs()[0]) * lookup[dim][byte].backend().size();
				uint8_t* bytes = (uint8_t*)lookup[dim][byte].backend().limbs();
				for (int pos = 0; pos < dims; ++pos)
				{
					int offset = dim * BYTE_COUNT * dims + byte * dims + pos;
					cpu_lookup_table_[offset] = bytes[pos];
				}
			}
		}
		for (int i = 0; i < dims; ++i)
			delete[] lookup[i];
		delete lookup;
	}


private:
	std::vector<uint8_t> cpu_lookup_table_;
	viennacl::vector<uint32_t> lookup_table_;

};


#endif

