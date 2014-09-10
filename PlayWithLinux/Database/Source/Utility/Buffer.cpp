#include "Buffer.h"

namespace vl
{
	namespace database
	{
/***********************************************************************
BufferManager
***********************************************************************/

		BufferManager::BufferManager(vuint64_t _pageSize, vuint64_t _maxCacheSize)
			:pageSize(_pageSize)
			,maxCacheSize(_maxCacheSize > _pageSize ? _maxCacheSize : _pageSize)
			,pageSizeBits(0)
		{
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
		}

		vuint64_t BufferManager::GetPageSize()
		{
			return pageSize;
		}

		vuint64_t BufferManager::GetMaxCacheSize()
		{
			return maxCacheSize;
		}

		BufferSource BufferManager::LoadMemorySource()
		{
			return BufferSource::Invalid();
		}

		BufferSource BufferManager::LoadFileSource(const WString& fileName)
		{
			return BufferSource::Invalid();
		}

		BufferSource BufferManager::CreateFileSource(const WString& fileName, bool createNew)
		{
			return BufferSource::Invalid();
		}

		bool BufferManager::UnloadSource(BufferSource source)
		{
			return false;
		}

		WString BufferManager::GetSourceFileName(BufferSource source)
		{
			return L"";
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
