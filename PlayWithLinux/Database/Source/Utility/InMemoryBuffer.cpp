#include "Buffer.h"
#include <stdlib.h>
#include <time.h>

namespace vl
{
	namespace database
	{
		using namespace collections;

/***********************************************************************
InMemoryBufferSource
***********************************************************************/

		class InMemoryBufferSource : public Object, public IBufferSource
		{
			typedef collections::List<Ptr<BufferPageDesc>>	PageList;
			typedef collections::List<vuint64_t>			PageIdList;
		private:
			BufferSource		source;
			volatile vuint64_t*	totalUsedPages;
			vuint64_t			pageSize;
			SpinLock			lock;
			PageList			pages;
			PageIdList			freePages;

		public:
			InMemoryBufferSource(BufferSource _source, volatile vuint64_t* _totalUsedPages, vuint64_t _pageSize)
				:source(_source)
				,totalUsedPages(_totalUsedPages)
				,pageSize(_pageSize)
			{
			}

			void Unload()override
			{
				FOREACH(Ptr<BufferPageDesc>, pageDesc, pages)
				{
					if (pageDesc)
					{
						free(pageDesc->address);
					}
				}
			}

			BufferSource GetBufferSource()override
			{
				return source;
			}

			SpinLock& GetLock()override
			{
				return lock;
			}

			WString GetFileName()override
			{
				return L"";
			}

			Ptr<BufferPageDesc> MapPage(BufferPage page)
			{
				if (page.index > pages.Count())
				{
					return nullptr;
				}
				else if (page.index == pages.Count() || !pages[page.index])
				{
					auto address = malloc(pageSize);
					if (!address) return nullptr;

					auto pageDesc = MakePtr<BufferPageDesc>();
					pageDesc->address = address;
					pageDesc->offset = page.index * pageSize;
					pageDesc->lastAccessTime = (vuint64_t)time(nullptr);

					if (page.index == pages.Count())
					{
						pages.Add(pageDesc);
					}
					else
					{
						pages[page.index] = pageDesc;
					}
					INCRC(totalUsedPages);
					return pageDesc;
				}
				else
				{
					auto pageDesc = pages[page.index];
					pageDesc->lastAccessTime = (vuint64_t)time(nullptr);
					return pageDesc;
				}
			}

			bool UnmapPage(BufferPage page)override
			{
				if (page.index >= pages.Count())
				{
					return false;
				}

				auto pageDesc = pages[page.index];
				if (!pageDesc || pageDesc->locked)
				{
					return false;
				}

				free(pageDesc->address);
				pages[page.index] = 0;
				freePages.Add(page.index);
				DECRC(totalUsedPages);
				return true;
			}

			BufferPage AllocatePage()override
			{
				BufferPage page = BufferPage::Invalid();
				if (freePages.Count() > 0)
				{
					page.index = freePages[freePages.Count() - 1];
					freePages.RemoveAt(freePages.Count() - 1);
				}
				else
				{
					page.index = pages.Count();
				}

				if (MapPage(page))
				{
					return page;
				}
				else
				{
					return BufferPage::Invalid();
				}
			}

			bool FreePage(BufferPage page)override
			{
				return UnmapPage(page);
			}

			void* LockPage(BufferPage page)override
			{
				if (page.index >= pages.Count())
				{
					return nullptr;
				}

				auto pageDesc = pages[page.index];
				if (!pageDesc || pageDesc->locked)
				{
					return nullptr;
				}

				pageDesc->locked = true;
				return pageDesc->address;
			}

			bool UnlockPage(BufferPage page, void* address, bool persist)override
			{
				if (page.index >= pages.Count())
				{
					return false;
				}

				auto pageDesc = pages[page.index];
				if (!pageDesc || !pageDesc->locked || address != pageDesc->address)
				{
					return false;
				}

				pageDesc->locked = false;
				return true;
			}

			void FillUnmapPageCandidates(collections::List<BufferPageTimeTuple>& pages, vint expectCount)override
			{
			}
		};

		IBufferSource* CreateMemorySource(BufferSource source, volatile vuint64_t* totalUsedPages, vuint64_t pageSize)
		{
			return new InMemoryBufferSource(source, totalUsedPages, pageSize);
		}
	}
}
