/***********************************************************************
Vczh Library++ 3.0
Developer: Zihan Chen(vczh)
Database::Utility

***********************************************************************/

#ifndef VCZH_DATABASE_UTILITY_COMMON
#define VCZH_DATABASE_UTILITY_COMMON

#include "../DatabaseVlppReferences.h"

namespace vl
{
	namespace database
	{
#define DEFINE_INDEX_TYPE(NAME, TYPE)							\
		struct NAME												\
		{														\
			typedef TYPE		IndexType;						\
			TYPE				index;							\
																\
			bool IsValid()const									\
			{													\
				return index != ~(TYPE)0;						\
			}													\
																\
			static NAME Invalid()								\
			{													\
				NAME source{~(TYPE)0};							\
				return source;									\
			}													\
		}														\

		DEFINE_INDEX_TYPE(BufferSource, vint32_t);
		DEFINE_INDEX_TYPE(BufferPage, vuint64_t);
		DEFINE_INDEX_TYPE(BufferPointer, vuint64_t);
#undef DEFINE_INDEX_TYPE

		template<typename T>
		T IntUpperBound(T size, T divisor)
		{
			return size + (divisor - (size % divisor)) % divisor;
		}
	}
}

#endif
