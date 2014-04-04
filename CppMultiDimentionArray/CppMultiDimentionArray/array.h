#pragma once

#include <memory.h>

namespace vczh
{
	template<int _Dim>
	struct array_size
	{
		int						size;
		int						total;
		array_size<_Dim-1>		remains;

		array_size(int _size, const array_size<_Dim-1>& _remains)
			:size(_size)
			,total(_size*_remains.total)
			,remains(_remains)
		{
		}

		array_size<_Dim+1> operator,(int new_size)
		{
			return array_size<_Dim+1>(size, (remains,new_size));
		}
	};

	template<>
	struct array_size<0>
	{
		int total;

		array_size()
			:total(1)
		{
		}

		array_size<1> operator,(int new_size)
		{
			return array_size<1>(new_size, array_size<0>());
		}
	};

	typedef array_size<0> array_sizes;

	//-----------------------------------------------------------------

	template<typename T, int _Dim>
	class array_ref
	{
	private:
		const array_size<_Dim>&		sizes;
		T*							data;
	public:
		array_ref(const array_size<_Dim>& _sizes, T* _data)
			:sizes(_sizes)
			,data(_data)
		{
		}

		array_ref<T, _Dim-1> operator[](int index)
		{
			return array_ref<T, _Dim-1>(sizes.remains, data+(index*sizes.remains.total));
		}

		int size()
		{
			return sizes.size;
		}

		int total_size()
		{
			return sizes.total;
		}
	};

	template<typename T>
	class array_ref<T, 1>
	{
	private:
		const array_size<1>&		sizes;
		T*							data;
	public:
		array_ref(const array_size<1>& _sizes, T* _data)
			:sizes(_sizes)
			,data(_data)
		{
		}

		T& operator[](int index)
		{
			return data[index];
		}

		int size()
		{
			return sizes.size;
		}

		int total_size()
		{
			return sizes.total;
		}
	};

	//-----------------------------------------------------------------

	template<typename T, int _Dim>
	class array
	{
	private:
		array_size<_Dim>			sizes;
		T*							data;
	public:
		array(const array_size<_Dim>& _sizes)
			:sizes(_sizes)
			,data(new T[_sizes.total])
		{
		}

		array(const array<T, _Dim>& arr)
			:sizes(arr.sizes)
			,data(new T[sizes.total])
		{
			for(int i=0;i<sizes.total;i++)
			{
				data[i]=arr.data[i];
			}
		}

		array(array<T, _Dim>&& arr)
			:sizes(arr.sizes)
			,data(arr.data)
		{
			arr.data=0;
		}

		~array()
		{
			if(data)
			{
				delete[] data;
			}
		}

		array_ref<T, _Dim> ref()
		{
			return array_ref<T, _Dim>(sizes, data);
		}

		int size()
		{
			return sizes.size;
		}

		int total_size()
		{
			return sizes.total;
		}

		auto operator[](int index)
#ifdef _MSC_VER
			->decltype(ref()[index])
#else
			->decltype(this->ref()[index])
#endif
		{
			return ref()[index];
		}
	};
}
