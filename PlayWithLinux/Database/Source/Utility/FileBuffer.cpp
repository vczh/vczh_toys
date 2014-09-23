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

/*
 * Page Structure
 *		Initial Page	: [uint64 NextInitialPage][uint64 FreePageItems]{[uint64 FreePage] ...}
 *		Use Mask Page	: [uint64 NextUseMaskPage]{[bit FreePageMask] ...}
 *			FreePageMask 1=used, 0=free
 */

#define INDEX_INVALID (~(vuint64_t)0)
#define INDEX_PAGE_INITIAL 0
#define INDEX_PAGE_USEMASK 1

#define INDEX_INITIAL_NEXTINITIALPAGE 0
#define INDEX_INITIAL_FREEPAGEITEMS 1
#define INDEX_INITIAL_FREEPAGEITEMBEGIN 2

#define INDEX_USEMASK_NEXTUSEMASKPAGE 0
#define INDEX_USEMASK_USEMASKBEGIN 1

namespace vl
{
	namespace database
	{
		using namespace collections;

/***********************************************************************
FileBufferSource
***********************************************************************/

		class FileBufferSource : public Object, public IBufferSource
		{
			typedef collections::Dictionary<vuint64_t, Ptr<BufferPageDesc>>		PageMap;
			typedef collections::List<vuint64_t>								PageList;
		private:
			BufferSource			source;
			volatile vuint64_t*		totalUsedPages;
			vuint64_t				pageSize;
			SpinLock				lock;
			WString					fileName;
			int						fileDescriptor;

			PageMap					mappedPages;
			PageList				initialPages;
			PageList				useMaskPages;
			vint					activeInitialPageIndex;
			vuint64_t				totalPageCount;

			vuint64_t				initialPageItemCount;
			vuint64_t				useMaskPageItemCount;

		public:

			FileBufferSource(BufferSource _source, volatile vuint64_t* _totalUsedPages, vuint64_t _pageSize, const WString& _fileName, int _fileDescriptor)
				:source(_source)
				,totalUsedPages(_totalUsedPages)
				,pageSize(_pageSize)
				,fileName(_fileName)
				,fileDescriptor(_fileDescriptor)
				,initialPageItemCount((_pageSize - INDEX_INITIAL_FREEPAGEITEMBEGIN * sizeof(vuint64_t)) / sizeof(vuint64_t))
				,useMaskPageItemCount((_pageSize - INDEX_USEMASK_USEMASKBEGIN * sizeof(vuint64_t)) / sizeof(vuint64_t))
				,activeInitialPageIndex(-1)
				,totalPageCount(0)
			{
			}

			void Unload()override
			{
				FOREACH(Ptr<BufferPageDesc>, pageDesc, mappedPages.Values())
				{
					munmap(pageDesc->address, pageSize);
				}
				close(fileDescriptor);
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
				return fileName;
			}

			Ptr<BufferPageDesc> MapPage(BufferPage page)
			{
				vint index =mappedPages.Keys().IndexOf(page.index);
				if (index == -1)
				{
					vuint64_t offset = page.index * pageSize;
					struct stat fileState;	
					if (fstat(fileDescriptor, &fileState) == -1)
					{
						return 0;
					}
					if (fileState.st_size < offset + pageSize)
					{
						if (fileState.st_size != offset)
						{
							return 0;
						}
						ftruncate(fileDescriptor, offset + pageSize);
						totalPageCount = offset / pageSize + 1;
					}

					void* address = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, offset);
					if (address == MAP_FAILED)
					{
						return nullptr;
					}
					
					auto pageDesc = MakePtr<BufferPageDesc>();
					pageDesc->address = address;
					pageDesc->offset = offset;
					pageDesc->lastAccessTime = (vuint64_t)time(nullptr);
					mappedPages.Add(page.index, pageDesc);
					INCRC(totalUsedPages);
					return pageDesc;
				}
				else
				{
					auto pageDesc = mappedPages.Values()[index];
					pageDesc->lastAccessTime = (vuint64_t)time(nullptr);
					return pageDesc;
				}
			}

			bool UnmapPage(BufferPage page)override
			{
				vint index = mappedPages.Keys().IndexOf(page.index);
				if (index == -1)
				{
					return false;
				}

				auto pageDesc = mappedPages.Values()[index];
				if (pageDesc->locked)
				{
					return false;
				}
				munmap(pageDesc->address, pageSize);

				mappedPages.Remove(page.index);
				DECRC(totalUsedPages);
				return true;
			}
			
			void PushFreePage(BufferPage page)
			{
				BufferPage initialPage{initialPages[activeInitialPageIndex]};
				auto pageDesc = MapPage(initialPage);
				vuint64_t* numbers = (vuint64_t*)pageDesc->address;
				vuint64_t& count = numbers[INDEX_INITIAL_FREEPAGEITEMS];
				if (count == initialPageItemCount)
				{
					if (activeInitialPageIndex == initialPages.Count() - 1)
					{
						BufferPage newInitialPage{totalPageCount};
						numbers[INDEX_INITIAL_NEXTINITIALPAGE] = newInitialPage.index;
						msync(numbers, pageSize, MS_SYNC);

						auto newPageDesc = MapPage(newInitialPage);
						numbers = (vuint64_t*)newPageDesc->address;
						memset(numbers, 0, pageSize);
						numbers[INDEX_INITIAL_NEXTINITIALPAGE] = INDEX_INVALID;
						numbers[INDEX_INITIAL_FREEPAGEITEMS] = 1;
						numbers[INDEX_INITIAL_FREEPAGEITEMBEGIN] = page.index;
						msync(numbers, pageSize, MS_SYNC);
						initialPages.Add(newInitialPage.index);
						SetUseMask(newInitialPage, true);
					}
					else
					{
						BufferPage newInitialPage{initialPages[activeInitialPageIndex + 1]};
						auto newPageDesc = MapPage(newInitialPage);
						numbers = (vuint64_t*)newPageDesc->address;
						numbers[INDEX_INITIAL_FREEPAGEITEMS] = 1;
						numbers[INDEX_INITIAL_FREEPAGEITEMBEGIN] = page.index;
						msync(numbers, pageSize, MS_SYNC);
					}
					activeInitialPageIndex++;
				}
				else
				{
					numbers[INDEX_INITIAL_FREEPAGEITEMBEGIN + count] = page.index;
					count++;
					msync(numbers, pageSize, MS_SYNC);
				}
			}

			BufferPage PopFreePage()
			{
				BufferPage page = BufferPage::Invalid();
				BufferPage initialPage{initialPages[activeInitialPageIndex]};
				auto pageDesc = MapPage(initialPage);
				vuint64_t* numbers = (vuint64_t*)pageDesc->address;
				vuint64_t& count = numbers[INDEX_INITIAL_FREEPAGEITEMS];

				if (count == 0 && initialPage.index == INDEX_PAGE_INITIAL)
				{
					return page;
				}
				count--;
				page.index = numbers[INDEX_INITIAL_FREEPAGEITEMBEGIN + count];
				msync(numbers, pageSize, MS_SYNC);

				if (count == 0)
				{
					activeInitialPageIndex--;
				}
				return page;
			}

			bool GetUseMask(BufferPage page)
			{
				auto useMaskPageBits = 8 * sizeof(vuint64_t);
				auto useMaskPageIndex = page.index / (useMaskPageBits * useMaskPageItemCount);
				auto useMaskPageBitIndex = page.index % (useMaskPageBits * useMaskPageItemCount);
				auto useMaskPageItem = INDEX_USEMASK_USEMASKBEGIN + useMaskPageBitIndex / useMaskPageBits;
				auto useMaskPageShift = useMaskPageBitIndex % useMaskPageBits;

				BufferPage useMaskPage{useMaskPages[useMaskPageIndex]};
				auto pageDesc = MapPage(useMaskPage);
				vuint64_t* numbers = (vuint64_t*)pageDesc->address;
				auto& item = numbers[useMaskPageItem];
				bool result = ((item >> useMaskPageShift) & ((vuint64_t)1)) == 1;
				msync(numbers, pageSize, MS_SYNC);
				return result;
			}
			
			void SetUseMask(BufferPage page, bool available)
			{
				auto useMaskPageBits = 8 * sizeof(vuint64_t);
				auto useMaskPageIndex = page.index / (useMaskPageBits * useMaskPageItemCount);
				auto useMaskPageBitIndex = page.index % (useMaskPageBits * useMaskPageItemCount);
				auto useMaskPageItem = INDEX_USEMASK_USEMASKBEGIN + useMaskPageBitIndex / useMaskPageBits;
				auto useMaskPageShift = useMaskPageBitIndex % useMaskPageBits;
				bool newPage = false;

				BufferPage useMaskPage = BufferPage::Invalid();

				if (useMaskPageIndex == useMaskPages.Count())
				{
					newPage = true;
					BufferPage lastPage{useMaskPages[useMaskPageIndex - 1]};
					useMaskPage = AppendPage();
					useMaskPages.Add(useMaskPage.index);

					auto pageDesc = MapPage(lastPage);
					vuint64_t* numbers = (vuint64_t*)pageDesc->address;
					numbers[INDEX_USEMASK_NEXTUSEMASKPAGE] = useMaskPage.index;
					msync(numbers, pageSize, MS_SYNC);
				}
				else
				{
					useMaskPage.index = useMaskPages[useMaskPageIndex];
				}

				auto pageDesc = MapPage(useMaskPage);
				vuint64_t* numbers = (vuint64_t*)pageDesc->address;
				if (newPage)
				{
					numbers[INDEX_USEMASK_NEXTUSEMASKPAGE] = INDEX_INVALID;
				}

				auto& item = numbers[useMaskPageItem];
				if (available)
				{
					vuint64_t mask = ((vuint64_t)1) << useMaskPageShift;
					item |= mask;
				}
				else
				{
					vuint64_t mask = ~(((vuint64_t)1) << useMaskPageShift);
					item &= mask;
				}
				msync(numbers, pageSize, MS_SYNC);
			}

			BufferPage AppendPage()
			{
				BufferPage page{totalPageCount};
				if (auto pageDesc = MapPage(page))
				{
					SetUseMask(page, true);
					return page;
				}
				else
				{
					return BufferPage::Invalid();
				}
			}

			BufferPage AllocatePage()override
			{
				BufferPage page = PopFreePage();
				if (page.index == -1)
				{
					return AppendPage();
				}
				else
				{
					SetUseMask(page, true);
					return page;
				}
			}

			bool FreePage(BufferPage page)override
			{
				if (!GetUseMask(page)) return false;
				vint index = mappedPages.Keys().IndexOf(page.index);
				if (index != -1)
				{
					if (!UnmapPage(page))
					{
						return false;
					}
				}
				PushFreePage(page);
				SetUseMask(page, false);
				return true;
			}

			void* LockPage(BufferPage page)override
			{
				if (page.index >= totalPageCount)
				{
					return nullptr;
				}
				if (!GetUseMask(page)) return nullptr;
				if (auto pageDesc = MapPage(page))
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

			bool UnlockPage(BufferPage page, void* buffer, bool persist)override
			{
				vint index =mappedPages.Keys().IndexOf(page.index);
				if (index == -1) return false;

				auto pageDesc = mappedPages.Values()[index];
				if (pageDesc->address != buffer) return false;
				if (!pageDesc->locked) return false;

				if (persist)
				{
					msync(pageDesc->address, pageSize, MS_SYNC);
				}
				pageDesc->locked = false;
				return true;
			}	

			void InitializeEmptySource()
			{
				initialPages.Clear();
				useMaskPages.Clear();
				{
					BufferPage page{INDEX_PAGE_INITIAL};
					auto pageDesc = MapPage(page);
					vuint64_t* numbers = (vuint64_t*)pageDesc->address;
					memset(numbers, 0, pageSize);
					numbers[INDEX_INITIAL_NEXTINITIALPAGE] = INDEX_INVALID;
					numbers[INDEX_INITIAL_FREEPAGEITEMS] = 0;
					msync(numbers, pageSize, MS_SYNC);
					initialPages.Add(page.index);
					activeInitialPageIndex = 0;
					totalPageCount = 2;
				}
				{
					BufferPage page{INDEX_PAGE_USEMASK};
					auto pageDesc = MapPage(page);
					vuint64_t* numbers = (vuint64_t*)pageDesc->address;
					memset(numbers, 0, pageSize);
					numbers[INDEX_USEMASK_NEXTUSEMASKPAGE] = INDEX_INVALID;
					msync(numbers, pageSize, MS_SYNC);
					useMaskPages.Add(page.index);
				}
				{
					BufferPage page{INDEX_PAGE_INITIAL};
					SetUseMask(page, true);
				}
				{
					BufferPage page{INDEX_PAGE_USEMASK};
					SetUseMask(page, true);
				}
			}

			void InitializeExistingSource()
			{
				initialPages.Clear();
				useMaskPages.Clear();
				{
					struct stat fileState;
					fstat(fileDescriptor, &fileState);
					totalPageCount = fileState.st_size / pageSize;
				}
				{
					BufferPage page{INDEX_PAGE_INITIAL};
					
					while(page.index != INDEX_INVALID)
					{
						initialPages.Add(page.index);
						auto pageDesc = MapPage(page);
						vuint64_t* numbers = (vuint64_t*)pageDesc->address;
						page.index = numbers[INDEX_INITIAL_NEXTINITIALPAGE];

						if (numbers[INDEX_INITIAL_FREEPAGEITEMS] != 0)
						{
							activeInitialPageIndex = initialPages.Count() - 1;
						}
					}
					
					if (activeInitialPageIndex == -1)
					{
						activeInitialPageIndex = 0;
					}
				}
				{
					BufferPage page{INDEX_PAGE_USEMASK};
					
					while(page.index != INDEX_INVALID)
					{
						useMaskPages.Add(page.index);
						auto pageDesc = MapPage(page);
						vuint64_t* numbers = (vuint64_t*)pageDesc->address;
						page.index = numbers[INDEX_USEMASK_NEXTUSEMASKPAGE];
					}
				}
			}

			void FillUnmapPageCandidates(collections::List<BufferPageTimeTuple>& pages, vint expectCount)override
			{
				vint mappedCount = mappedPages.Count();
				if (mappedCount == 0) return;

				Array<BufferPageTimeTuple> tuples(mappedCount);
				vint usedCount = 0;
				for (vint i = 0; i < mappedCount; i++)
				{
					auto key = mappedPages.Keys()[i];
					auto value = mappedPages.Values()[i];
					if (!value->locked)
					{
						BufferPage page{key};
						tuples[usedCount++] = BufferPageTimeTuple(source, page, value->lastAccessTime);
					}
				}

				if (tuples.Count() > 0)
				{
					SortLambda(&tuples[0], usedCount, [](const BufferPageTimeTuple& t1, const BufferPageTimeTuple& t2)
					{
						if (t1.f2 < t2.f2) return -1;
						else if (t1.f2 > t2.f2) return 1;
						else return 0;
					});

					vint copyCount = usedCount < expectCount ? usedCount : expectCount;
					for (vint i = 0; i < copyCount; i++)
					{
						pages.Add(tuples[i]);
					}
				}
			}
		};

		IBufferSource* CreateFileSource(BufferSource source, volatile vuint64_t* totalUsedPages, vuint64_t pageSize, const WString& fileName, bool createNew)
		{
			int fileDescriptor = 0;
			if (createNew)
			{
				auto mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
				fileDescriptor = open(wtoa(fileName).Buffer(), O_CREAT | O_TRUNC | O_RDWR, mode);
			}
			else
			{
				fileDescriptor = open(wtoa(fileName).Buffer(), O_RDWR);
			}

			if (fileDescriptor == -1)
			{
				return nullptr;
			}
			else
			{
				auto result = new FileBufferSource(source, totalUsedPages, pageSize, fileName, fileDescriptor);
				if (createNew)
				{
					result->InitializeEmptySource();
				}
				else
				{
					result->InitializeExistingSource();
				}
				return result;
			}
		}
	}
}
