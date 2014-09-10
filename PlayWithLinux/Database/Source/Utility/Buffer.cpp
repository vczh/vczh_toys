#include "Buffer.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace vl
{
	namespace database
	{
		using namespace collections;
/***********************************************************************
BufferManager
***********************************************************************/

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
			return source;
		}

		bool BufferManager::UnloadSource(BufferSource source)
		{
			vint index = sourceDescs.Keys().IndexOf(source.index);
			if (index == -1) return false;
			auto sourceDesc = sourceDescs.Values()[index];

			FOREACH(void*, address, sourceDesc->pages.Values())
			{
				pageDescs.Remove(address);
				if (sourceDesc->inMemory)
				{
					free(address);
				}
				else
				{
					munmap(address, pageSize);
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
			return nullptr;
		}

		bool BufferManager::UnlockPage(BufferSource source, BufferPage page, void* buffer)
		{
			return false;
		}

		BufferPage BufferManager::AllocatePage(BufferSource source)
		{
			return BufferPage::Invalid();
		}

		bool BufferManager::FreePage(BufferSource source, BufferPage page)
		{
			return false;
		}

		bool BufferManager::EncodePointer(BufferPointer& pointer, BufferPage page, vuint64_t offset)
		{
			return false;
		}

		bool BufferManager::DecodePointer(BufferPointer pointer, BufferPage& page, vuint64_t& offset)
		{
			return false;
		}
	}
}
