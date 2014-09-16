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
					struct stat fileState;	
					if (fstat(sourceDesc->fileDescriptor, &fileState) == -1)
					{
						return 0;
					}
					if (fileState.st_size < offset + pageSize)
					{
						ftruncate(sourceDesc->fileDescriptor, offset + pageSize);
					}

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
			vuint64_t* numbers = (vuint64_t*)LockPage(source, sourceDesc, initialPage);

			BufferPage page{numbers[INDEX_INITIAL_TOTALPAGECOUNT]};
			if (auto pageDesc = MapPage(source, sourceDesc, page))
			{
				numbers[INDEX_INITIAL_TOTALPAGECOUNT]++;
				memset(pageDesc->address, 0, pageSize);
				UnlockPage(source, sourceDesc, initialPage, numbers, true);
			}
			else
			{
				UnlockPage(source, sourceDesc, initialPage, numbers, false);
			}
			return page;
		}

		BufferPage BufferManager::AllocatePage(BufferSource source, Ptr<SourceDesc> sourceDesc)
		{
			BufferPage page = BufferPage::Invalid();
			BufferPage freePage{0};
			BufferPage previousFreePage = BufferPage::Invalid();
			while (page.index == INDEX_INVALID && freePage.index != INDEX_INVALID)
			{
				previousFreePage = freePage;
				vuint64_t* numbers = (vuint64_t*)LockPage(source, sourceDesc, freePage);
				if (!numbers) return BufferPage::Invalid();

				auto next = numbers[INDEX_INITIAL_NEXTFREEPAGE];
				if (next == INDEX_INVALID)
				{
					vuint64_t& count = numbers[INDEX_INITIAL_AVAILABLEITEMS];
					if (count > 0)
					{
						page.index = numbers[count - 1 + INDEX_INITIAL_AVAILABLEITEMBEGIN];
						count--;
						UnlockPage(source, sourceDesc, freePage, numbers, true);

						if (count == 0 && previousFreePage.index != INDEX_INVALID)
						{
							numbers = (vuint64_t*)LockPage(source, sourceDesc, previousFreePage);
							numbers[INDEX_INITIAL_NEXTFREEPAGE] = INDEX_INVALID;
							UnlockPage(source, sourceDesc, previousFreePage, numbers, false);
							FreePage(source, freePage);
							break;
						}
					}
				}
				freePage.index = next;
				UnlockPage(source, sourceDesc, previousFreePage, numbers, false);
			}

			if (page.index == -1)
			{
				return AppendPage(source, sourceDesc);
			}
			else
			{
				return page;
			}
		}

		bool BufferManager::FreePage(BufferSource source, Ptr<SourceDesc> sourceDesc, BufferPage page)
		{
			vint index = sourceDesc->mappedPages.Keys().IndexOf(page.index);
			if (index == -1)
			{
				if (sourceDesc->inMemory)
				{
					return false;
				}
			}
			else
			{
				auto pageDesc = sourceDesc->mappedPages.Values()[index];
				if (pageDesc->locked)
				{
					return false;
				}
			}

			vuint64_t maxAvailableItems = (pageSize - INDEX_INITIAL_AVAILABLEITEMBEGIN * sizeof(vuint64_t)) / sizeof(vuint64_t);
			BufferPage freePage{0};
			while (freePage.index != INDEX_INVALID)
			{
				vuint64_t* numbers = (vuint64_t*)LockPage(source, sourceDesc, freePage);
				if (!numbers)
				{
					return false;
				}

				vuint64_t& count = numbers[INDEX_INITIAL_AVAILABLEITEMS];
				if (count < maxAvailableItems)
				{
					numbers[count + INDEX_INITIAL_AVAILABLEITEMBEGIN] = page.index;
					count++;
					UnlockPage(source, sourceDesc, freePage, numbers, true);
					return true;
				}
				else if (numbers[INDEX_INITIAL_NEXTFREEPAGE] != INDEX_INVALID)
				{
					vuint64_t index = numbers[INDEX_INITIAL_NEXTFREEPAGE];
					UnlockPage(source, sourceDesc, freePage, numbers, false);
					freePage.index = index;
				}
				else
				{
					BufferPage nextFreePage = AppendPage(source, sourceDesc);
					if (nextFreePage.index != INDEX_INVALID)
					{
						numbers[INDEX_INITIAL_NEXTFREEPAGE] = nextFreePage.index;
						UnlockPage(source, sourceDesc, freePage, numbers, true);

						numbers = (vuint64_t*)LockPage(source, sourceDesc, nextFreePage);
						numbers[INDEX_INITIAL_NEXTFREEPAGE] = INDEX_INVALID;
						numbers[INDEX_INITIAL_AVAILABLEITEMS] = 0;
						UnlockPage(source, sourceDesc, nextFreePage, numbers, true);
						return true;
					}
					else
					{
						UnlockPage(source, sourceDesc, freePage, numbers, false);
						break;
					}
				}
			}

			return false;
		}

		void* BufferManager::LockPage(BufferSource source, Ptr<SourceDesc> sourceDesc, BufferPage page)
		{
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

		bool BufferManager::UnlockPage(BufferSource source, Ptr<SourceDesc> sourceDesc, BufferPage page, void* buffer, bool persist)
		{
			vint index = sourceDesc->mappedPages.Keys().IndexOf(page.index);
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

	void BufferManager::InitializeSource(BufferSource source, Ptr<SourceDesc> sourceDesc)
		{
			BufferPage page{0};
			auto pageDesc = MapPage(source, sourceDesc, page);
			vuint64_t* numbers = (vuint64_t*)pageDesc->address;
			memset(numbers, 0, pageSize);
			numbers[INDEX_INITIAL_TOTALPAGECOUNT] = 1;
			numbers[INDEX_INITIAL_NEXTFREEPAGE] = INDEX_INVALID;
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
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			auto desc = MakePtr<SourceDesc>();
			desc->inMemory = true;

			SPIN_LOCK(lock)
			{
				sourceDescs.Add(source.index, desc);
			}
			SPIN_LOCK(desc->lock)
			{
				InitializeSource(source, desc);
			}
			return source;
		}

		BufferSource BufferManager::LoadFileSource(const WString& fileName, bool createNew)
		{
			BufferSource source{(BufferSource::IndexType)INCRC(&usedSourceIndex)};
			auto desc = MakePtr<SourceDesc>();
			desc->inMemory = false;
			desc->fileName = fileName;

			if (createNew)
			{
				auto mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
				desc->fileDescriptor = open(wtoa(fileName).Buffer(), O_CREAT | O_TRUNC | O_RDWR, mode);
			}
			else
			{
				desc->fileDescriptor = open(wtoa(fileName).Buffer(), O_RDWR);
			}
			if (desc->fileDescriptor == -1)
			{
				return BufferSource::Invalid();
			}

			SPIN_LOCK(lock)
			{
				sourceDescs.Add(source.index, desc);
			}
			if (createNew)
			{
				SPIN_LOCK(desc->lock)
				{
					InitializeSource(source, desc);
				}
			}
			return source;
		}

		bool BufferManager::UnloadSource(BufferSource source)
		{
			Ptr<SourceDesc> sourceDesc;
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return false;
				sourceDesc = sourceDescs.Values()[index];
				sourceDescs.Remove(source.index);
			}

			SPIN_LOCK(sourceDesc->lock)
			{
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
			}

			if (!sourceDesc->inMemory)
			{
				close(sourceDesc->fileDescriptor);
			}
			return true;
		}

		WString BufferManager::GetSourceFileName(BufferSource source)
		{
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return L"";
				return sourceDescs.Values()[index]->fileName;
			}
		}

		void* BufferManager::LockPage(BufferSource source, BufferPage page)
		{
			Ptr<SourceDesc> sourceDesc;
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return nullptr;
				sourceDesc = sourceDescs.Values()[index];
			}

			SPIN_LOCK(sourceDesc->lock)
			{
				return LockPage(source, sourceDesc, page);
			}
		}

		bool BufferManager::UnlockPage(BufferSource source, BufferPage page, void* buffer, bool persist)
		{
			Ptr<SourceDesc> sourceDesc;
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return false;
				sourceDesc = sourceDescs.Values()[index];
			}

			SPIN_LOCK(sourceDesc->lock)
			{
				return UnlockPage(source, sourceDesc, page, buffer, persist);
			}
		}

		BufferPage BufferManager::AllocatePage(BufferSource source)
		{
			Ptr<SourceDesc> sourceDesc;
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return BufferPage::Invalid();
				sourceDesc = sourceDescs.Values()[index];
			}

			SPIN_LOCK(sourceDesc->lock)
			{
				return AllocatePage(source, sourceDesc);
			}
		}

		bool BufferManager::FreePage(BufferSource source, BufferPage page)
		{
			Ptr<SourceDesc> sourceDesc;
			SPIN_LOCK(lock)
			{
				vint index = sourceDescs.Keys().IndexOf(source.index);
				if (index == -1) return false;
				sourceDesc = sourceDescs.Values()[index];
			}

			SPIN_LOCK(sourceDesc->lock)
			{
				return FreePage(source, sourceDesc, page);
			}
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
