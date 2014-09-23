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

namespace vl
{
	namespace database
	{
		using namespace collections;

		extern IBufferSource*		CreateMemorySource(BufferSource source, volatile vuint64_t* totalCachedPages, vuint64_t pageSize);
		extern IBufferSource*		CreateFileSource(BufferSource source, volatile vuint64_t* totalCachedPages, vuint64_t pageSize, const WString& fileName, bool createNew);

/***********************************************************************
BufferManager
***********************************************************************/

		void BufferManager::SwapCacheIfNecessary()
		{
			if (totalCachedPages > cachePageCount)
			{
				SPIN_LOCK(lock)
				{
					vuint64_t remainPage = cachePageCount / 4 * 3;
					vuint64_t expectPage = totalCachedPages - remainPage;
					List<IBufferSource::BufferPageTimeTuple> pages;
					FOREACH(Ptr<IBufferSource>, source, sources.Values())
					{
						source->FillUnmapPageCandidates(pages, expectPage);
					}

					if (pages.Count() > 0)
					{
						SortLambda(&pages[0], pages.Count(), [](const IBufferSource::BufferPageTimeTuple& t1, const IBufferSource::BufferPageTimeTuple& t2)
						{
							if (t1.f2 < t2.f2) return -1;
							else if (t1.f2 > t2.f2) return 1;
							else return 0;
						});

						vint count = pages.Count() < expectPage ? pages.Count() : expectPage;
						for (vint i = 0; i < count; i++)
						{
							auto tuple = pages[i];
							auto source = sources[tuple.f0.index];
							SPIN_LOCK(source->GetLock())
							{
								source->UnmapPage(tuple.f1);
							}
						}

						if (totalCachedPages > cachePageCount)
						{
							throw 0;
						}
					}
				}
			}
		}

		BufferManager::BufferManager(vuint64_t _pageSize, vuint64_t _cachePageCount)
			:pageSize(_pageSize)
			,cachePageCount(_cachePageCount)
			,pageSizeBits(0)
			,totalCachedPages(0)
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

		vuint64_t BufferManager::GetCurrentlyCachedPageCount()
		{
			return totalCachedPages;
		}

		BufferSource BufferManager::LoadMemorySource()
		{
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			Ptr<IBufferSource> bs = CreateMemorySource(source, &totalCachedPages, pageSize);
			if (!bs)
			{
				return BufferSource::Invalid();
			}

			SPIN_LOCK(lock)
			{
				sources.Add(source.index, bs);
			}
			SwapCacheIfNecessary();
			return source;
		}

		BufferSource BufferManager::LoadFileSource(const WString& fileName, bool createNew)
		{
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			Ptr<IBufferSource> bs = CreateFileSource(source, &totalCachedPages, pageSize, fileName, createNew);
			if (!bs)
			{
				return BufferSource::Invalid();
			}

			SPIN_LOCK(lock)
			{
				sources.Add(source.index, bs);
			}
			SwapCacheIfNecessary();
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

			void* address = nullptr;
			SPIN_LOCK(bs->GetLock())
			{
				address = bs->LockPage(page);
			}
			SwapCacheIfNecessary();
			return address;
		}

		bool BufferManager::UnlockPage(BufferSource source, BufferPage page, void* buffer, bool persist)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, false);

			bool successful = false;
			SPIN_LOCK(bs->GetLock())
			{
				successful = bs->UnlockPage(page, buffer, persist);
			}
			SwapCacheIfNecessary();
			return successful;
		}

		BufferPage BufferManager::AllocatePage(BufferSource source)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, BufferPage::Invalid());

			BufferPage page;
			SPIN_LOCK(bs->GetLock())
			{
				page = bs->AllocatePage();
			}
			SwapCacheIfNecessary();
			return page;
		}

		bool BufferManager::FreePage(BufferSource source, BufferPage page)
		{
			TRY_GET_BUFFER_SOURCE(bs, source, false);
			
			bool successful = false;
			SPIN_LOCK(bs->GetLock())
			{
				successful = bs->FreePage(page);
			}
			SwapCacheIfNecessary();
			return successful;
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
