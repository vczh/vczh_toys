/***********************************************************************
Vczh Library++ 3.0
Developer: Zihan Chen(vczh)
Database::Utility

Interfaces:
***********************************************************************/

#ifndef VCZH_DATABASE_UTILITY_BUFFER
#define VCZH_DATABASE_UTILITY_BUFFER

#include "../DatabaseVlppReferences.h"

namespace vl
{
	namespace database
	{
#define DEFINE_INDEX_TYPE(NAME, TYPE)							\
		struct NAME												\
		{														\
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

		class BufferManager
		{
		private:
			vuint64_t			pageSize;
			vuint64_t			maxCacheSize;
			vuint64_t			pageSizeBits;

		public:
			BufferManager(vuint64_t _pageSize, vuint64_t _maxCacheSize);
			~BufferManager();

			vuint64_t			GetPageSize();
			vuint64_t			GetMaxCacheSize();

			BufferSource		LoadMemorySource();
			BufferSource		LoadFileSource(const WString& fileName);
			BufferSource		CreateFileSource(const WString& fileName, bool createNew);
			bool				UnloadSource(BufferSource source);
			WString				GetSourceFileName(BufferSource source);

			void*				LockPage(BufferSource source, BufferPage page);
			bool				UnlockPage(BufferSource source, BufferPage page, void* buffer);
			BufferPage			AllocatePage(BufferSource source);
			bool				FreePage(BufferSource source, BufferPage page);
			bool				EncodePointer(BufferPointer& pointer, BufferPage page, vuint64_t offset);
			bool				DecodePointer(BufferPointer pointer, BufferPage& page, vuint64_t& offset);
		};
	}
}

#endif
