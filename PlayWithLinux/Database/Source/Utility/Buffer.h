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

		class BufferManager
		{
		private:
			typedef collections::Dictionary<vuint64_t, void*>							OffsetPageMap;
			class SourceDesc
			{
			public:
				bool						inMemory;
				int							fileDescriptor = -1;
				WString						fileName;
				OffsetPageMap				pages;
			};
			class PageDesc
			{
			public:
				void*			address;
				BufferSource	source;
				vuint64_t		offset;
			};
			typedef collections::Dictionary<BufferSource::IndexType, Ptr<SourceDesc>>	SourceDescMap;
			typedef collections::Dictionary<void*, Ptr<PageDesc>>						MemoryDescMap;

		private:
			vuint64_t			pageSize;
			vuint64_t			cachePageCount;
			vuint64_t			pageSizeBits;

			SourceDescMap		sourceDescs;
			MemoryDescMap		pageDescs;

		public:
			BufferManager(vuint64_t _pageSize, vuint64_t _cachePageCount);
			~BufferManager();

			vuint64_t			GetPageSize();
			vuint64_t			GetCachePageCount();
			vuint64_t			GetCacheSize();

			BufferSource		LoadMemorySource();
			BufferSource		LoadFileSource(const WString& fileName, bool createNew);
			bool				UnloadSource(BufferSource source);
			WString				GetSourceFileName(BufferSource source);

			void*				LockPage(BufferSource source, BufferPage page);
			bool				UnlockPage(BufferSource source, BufferPage page, void* buffer);
			BufferPage			AllocatePage(BufferSource source);
			bool				FreePage(BufferSource source, BufferPage page);
			bool				EncodePointer(BufferPointer& pointer, BufferPage page, vuint64_t offset);
			bool				DecodePointer(BufferPointer pointer, BufferPage& page, vuint64_t& offset);
		};

		template<typename T>
		T IntUpperBound(T size, T divisor)
		{
			return size + (divisor - (size % divisor)) % divisor;
		}
	}
}

#endif
