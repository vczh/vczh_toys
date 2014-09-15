#include "Buffer.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define INDEX_INITIAL_TOTALPAGECOUNT 0
#define INDEX_INITIAL_NEXTFREEPAGE 1
#define INDEX_INITIAL_AVAILABLEITEMS 2
#define INDEX_INITIAL_AVAILABLEITEMBEGIN 3
#define INDEX_INVALID (~(vuint64_t)0)

namespace vl
{
	namespace database
	{
		using namespace collections;

		extern IBufferSource*		CreateMemorySource(BufferSource source, vuint64_t pageSize);
		extern IBufferSource*		CreateFileSource(BufferSource source, vuint64_t pageSize, const WString& fileName, bool createNew);

/***********************************************************************
BufferManager
***********************************************************************/

		BufferManager::BufferManager(vuint64_t _pageSize, vuint64_t _cachePageCount)
			:pageSize(_pageSize)
			,cachePageCount(_cachePageCount)
			,pageSizeBits(0)
			,usedSourceIndex(0)
		{
			vuint64_t systemPageSize = sysconf(_SC_PAGE_SIZE);
			pageSize = IntUpperBound(pageSize, systemPageSize);
			if (pageSize > 0)
			{
				pageSizeBits = sizeof(pageSize) * 8;
				while ((pageSize & (((vuint64_t)2) << (pageSizeBits - 1))) == 0)
				{
					pageSizeBits--;
				}
			}
		}

		BufferManager::~BufferManager()
		{
			FOREACH(Ptr<IBufferSource>, bs, sources.Values())
			{
				bs->Unload();
			}
		}

		vuint64_t BufferManager::GetPageSize()
		{
			return pageSize;
		}

		vuint64_t BufferManager::GetCachePageCount()
		{
			return cachePageCount;
		}

		vuint64_t BufferManager::GetCacheSize()
		{
			return pageSize * cachePageCount;
		}

		BufferSource BufferManager::LoadMemorySource()
		{
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			Ptr<IBufferSource> bs = CreateMemorySource(source, pageSize);
			if (!bs)
			{
				return BufferSource::Invalid();
			}

			SPIN_LOCK(lock)
			{
				sources.Add(source.index, bs);
			}
			return source;
		}

		BufferSource BufferManager::LoadFileSource(const WString& fileName, bool createNew)
		{
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			Ptr<IBufferSource> bs = CreateFileSource(source, pageSize, fileName, createNew);
			if (!bs)
			{
				return BufferSource::Invalid();
			}

			SPIN_LOCK(lock)
			{
				sources.Add(source.index, bs);
			}
			return source;
		}

#define TRY_GET_BUFFER_SOURCE(BS, SOURCE, FAILVALUE)					\
			Ptr<IBufferSource> BS;										\
			SPIN_LOCK(lock)												\
			{															\
				vint index = sources.Keys().IndexOf((SOURCE).index);	\
				if (index == -1) return FAILVALUE;						\
				BS = sources.Values()[index];							\
			}															\


		bool BufferManager::UnloadSource(BufferSource source)
		{
			Ptr<IBufferSource> bs;
			SPIN_LOCK(lock)
			{
				vint index = sources.Keys().IndexOf(source.index);
				if (index == -1) return false;
				bs = sources.Values()[index];
				sources.Remove(source.index);
			}

			SPIN_LOCK(bs->GetLock())
			{
				bs->Unload();
			}
			return true;
		}

		WString BufferManager::GetSourceFileName(BufferSource source)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, L"");
			return bs->GetFileName();
		}

		void* BufferManager::LockPage(BufferSource source, BufferPage page)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, nullptr);

			SPIN_LOCK(bs->GetLock())
			{
				return bs->LockPage(page);
			}
		}

		bool BufferManager::UnlockPage(BufferSource source, BufferPage page, void* buffer, bool persist)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, false);

			SPIN_LOCK(bs->GetLock())
			{
				return bs->UnlockPage(page, buffer, persist);
			}
		}

		BufferPage BufferManager::AllocatePage(BufferSource source)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, BufferPage::Invalid());

			SPIN_LOCK(bs->GetLock())
			{
				return bs->AllocatePage();
			}
		}

		bool BufferManager::FreePage(BufferSource source, BufferPage page)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, false);
			
			SPIN_LOCK(bs->GetLock())
			{
				return bs->FreePage(page);
			}
		}
		
#undef TRY_GET_BUFFER_SOURCE

		bool BufferManager::EncodePointer(BufferPointer& pointer, BufferPage page, vuint64_t offset)
		{
			if (offset >= pageSize) return false;
			pointer.index = (page.index << pageSizeBits) & offset;
			return true;
		}

		bool BufferManager::DecodePointer(BufferPointer pointer, BufferPage& page, vuint64_t& offset)
		{
			page.index = pointer.index >> pageSizeBits;
			offset = pointer.index << pageSizeBits >> pageSizeBits;
			return true;
		}
	}
}
