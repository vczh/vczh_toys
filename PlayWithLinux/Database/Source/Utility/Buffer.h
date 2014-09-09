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
		struct BufferSource
		{
			vint32_t			index;

			bool IsValid()const
			{
				return index != ~(decltype(index))0;
			}

			static BufferSource Invalid()
			{
				BufferSource source{~(decltype(index))0};
				return source;
			}
		};

		struct BufferPage
		{
			vuint64_t			index;

			bool IsValid()const
			{
				return index != ~(decltype(index))0;
			}

			static BufferPage Invalid()
			{
				BufferPage page{~(decltype(index))0};
				return page;
			}
		};

		struct BufferPointer
		{
			vuint64_t			index;

			bool IsValid()const
			{
				return index != ~(decltype(index))0;
			}
	
			static BufferPointer Invalid()
			{
				BufferPointer pointer{~(decltype(index))0};
				return pointer;
			}
		};

		class BufferManager
		{
		public:
			BufferManager(vuint64_t pageSize, vuint64_t maxCacheSize);
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
