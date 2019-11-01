#ifndef HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H	
#define HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H	

namespace hlasm_plugin::parser_library {

template<typename c_type, typename impl>
class c_view_array
{
public:
	c_view_array(const impl* data, size_t size) :
		data_(data), size_(size) {}

	//needs to be specialized for every use	
	c_type item(size_t index);
	size_t size()
	{
		return size_;
	}
private:
	const impl* data_;
	size_t size_;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H
