#include "Buffer.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#define INDEX_INITIAL_TOTALPAGECOUNT 0
#define INDEX_INITIAL_NEXTFREEPAGE 1
#define INDEX_INITIAL_AVAILABLEITEMS 2

namespace vl
{
	namespace database
	{
		using namespace collections;
/***********************************************************************
BufferManager
***********************************************************************/

		Ptr<BufferManager::PageDesc> BufferManager::MapPage(BufferSource source, Ptr<SourceDesc> sourceDesc, BufferPage page)
		{
			vint index = sourceDesc->mappedPages.Keys().IndexOf(page.index);
			if (index == -1)
			{
				vuint64_t offset = page.index * pageSize;
				void* address = nullptr;
				if (sourceDesc->inMemory)
				{
					address = malloc(pageSize);
				}
				else
				{
					address = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_SHARED, sourceDesc->fileDescriptor, offset);
					if (address == MAP_FAILED)
					{
						address = nullptr;
					}
				}
				if (!address) return 0;
				
				auto pageDesc = MakePtr<PageDesc>();
				pageDesc->address = address;
				pageDesc->source = source;
				pageDesc->offset = offset;
				pageDesc->lastAccessTime = (vuint64_t)time(nullptr);
				sourceDesc->mappedPages.Add(page.index, pageDesc);
				return pageDesc;
			}
			else
			{
				auto pageDesc = sourceDesc->mappedPages.Values()[index];
				pageDesc->lastAccessTime = (vuint64_t)time(nullptr);
				return pageDesc;
			}
		}

		bool BufferManager::UnmapPage(BufferSource source, Ptr<SourceDesc> sourceDesc, BufferPage page)
		{
			vint index = sourceDesc->mappedPages.Keys().IndexOf(page.index);
			if (index == -1) return false;

			auto pageDesc = sourceDesc->mappedPages.Values()[index];
			if (sourceDesc->inMemory)
			{
				free(pageDesc->address);
			}
			else
			{
				munmap(pageDesc->address, pageSize);
			}

			sourceDesc->mappedPages.Remove(page.index);
			return true;
		}

		BufferPage BufferManager::AppendPage(BufferSource source, Ptr<SourceDesc> sourceDesc)
		{
			BufferPage initialPage{0};
			vuint64_t* numbers = (vuint64_t*)LockPage(source, initialPage);

			BufferPage page{numbers[INDEX_INITIAL_TOTALPAGECOUNT]};
			if (auto pageDesc = MapPage(source, sourceDesc, page))
			{
				numbers[INDEX_INITIAL_TOTALPAGECOUNT]++;
				memset(pageDesc->address, 0, pageSize);
				UnlockPage(source, initialPage, numbers, true);
			}
			else
			{
				UnlockPage(source, initialPage, numbers, false);
			}
		}

		void BufferManager::InitializeSource(BufferSource source, Ptr<SourceDesc> sourceDesc)
		{
			BufferPage page{0};
			auto pageDesc = MapPage(source, sourceDesc, page);
			vuint64_t* numbers = (vuint64_t*)pageDesc->address;
			memset(numbers, 0, pageSize);
			numbers[INDEX_INITIAL_TOTALPAGECOUNT] = 1;
			numbers[INDEX_INITIAL_NEXTFREEPAGE] = -1;
			numbers[INDEX_INITIAL_AVAILABLEITEMS] = 0;

			if(!sourceDesc->inMemory)
			{
				msync(numbers, pageSize, MS_SYNC);
			}
		}

		BufferManager::BufferManager(vuint64_t _pageSize, vuint64_t _cachePageCount)
			:pageSize(_pageSize)
			,cachePageCount(_cachePageCount)
			,pageSizeBits(0)
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
			while (sourceDescs.Count() > 0)
			{
				BufferSource source{sourceDescs.Keys()[0]};
				UnloadSource(source);
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
			BufferSource source{(BufferSource::IndexType)sourceDescs.Count()};
			auto desc = MakePtr<SourceDesc>();
			desc->inMemory = true;
			sourceDescs.Add(source.index, desc);
			InitializeSource(source, desc);
			return source;
		}

		BufferSource BufferManager::LoadFileSource(const WString& fileName, bool createNew)
		{
			BufferSource source{(BufferSource::IndexType)sourceDescs.Count()};
			auto desc = MakePtr<SourceDesc>();
			desc->inMemory = false;
			desc->fileName = fileName;

			if (createNew)
			{
				desc->fileDescriptor = creat(wtoa(fileName).Buffer(), 0x555);
			}
			else
			{
				desc->fileDescriptor = open(wtoa(fileName).Buffer(), O_RDWR);
			}
			if (desc->fileDescriptor == -1)
			{
				return BufferSource::Invalid();
			}

			sourceDescs.Add(source.index, desc);
			if (createNew)
			{
				InitializeSource(source, desc);
			}
			return source;
		}

		bool BufferManager::UnloadSource(BufferSource source)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return false;
			auto sourceDesc = sourceDescs.Values()[index];

			FOREACH(Ptr<PageDesc>, pageDesc, sourceDesc->mappedPages.Values())
			{
				if (sourceDesc->inMemory)
				{
					free(pageDesc->address);
				}
				else
				{
					munmap(pageDesc->address, pageSize);
				}
			}

			sourceDescs.Remove(source.index);
			if (!sourceDesc->inMemory)
			{
				close(sourceDesc->fileDescriptor);
			}
			return true;
		}

		WString BufferManager::GetSourceFileName(BufferSource source)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return L"";
			return sourceDescs.Values()[index]->fileName;
		}

		void* BufferManager::LockPage(BufferSource source, BufferPage page)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return nullptr;

			auto sourceDesc = sourceDescs.Values()[index];
			BufferPage initialPage{0};
			if (sourceDesc->inMemory)
			{
				if (!sourceDesc->mappedPages.Keys().Contains(page.index))
				{
					return nullptr;
				}
			}
			else if (auto pageDesc = MapPage(source, sourceDesc, initialPage))
			{
				vuint64_t* numbers = (vuint64_t*)pageDesc->address;
				if (page.index >= numbers[INDEX_INITIAL_TOTALPAGECOUNT])
				{
					return nullptr;
				}
			}

			if (auto pageDesc = MapPage(source, sourceDesc, page))
			{
				if (pageDesc->locked) return nullptr;
				pageDesc->locked = true;
				return pageDesc->address;
			}
			else
			{
				return nullptr;
			}
		}

		bool BufferManager::UnlockPage(BufferSource source, BufferPage page, void* buffer, bool persist)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return false;

			auto sourceDesc = sourceDescs.Values()[index];
			index = sourceDesc->mappedPages.Keys().IndexOf(page.index);
			if (index == -1) return false;

			auto pageDesc = sourceDesc->mappedPages.Values()[index];
			if (pageDesc->address != buffer) return false;
			if (!pageDesc->locked) return false;

			if (persist && !sourceDesc->inMemory)
			{
				msync(pageDesc->address, pageSize, MS_SYNC);
			}
			pageDesc->locked = false;
			return true;
		}

		BufferPage BufferManager::AllocatePage(BufferSource source)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return BufferPage::Invalid();

			auto sourceDesc = sourceDescs.Values()[index];
			return BufferPage::Invalid();
		}

		bool BufferManager::FreePage(BufferSource source, BufferPage page)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return false;

			auto sourceDesc = sourceDescs.Values()[index];
			return false;
		}

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
