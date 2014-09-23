/***********************************************************************
Vczh Library++ 3.0
Developer: Zihan Chen(vczh)
Database::Utility

***********************************************************************/

#ifndef VCZH_DATABASE_UTILITY_BUFFER
#define VCZH_DATABASE_UTILITY_BUFFER

#include "Common.h"

namespace vl
{
	namespace database
	{
		class IBufferSource : public virtual Interface
		{
		public:
			typedef Tuple<BufferSource, BufferPage, vuint64_t>		BufferPageTimeTuple;

			virtual void			Unload() = 0;
			virtual BufferSource	GetBufferSource() = 0;
			virtual SpinLock&		GetLock() = 0;
			virtual WString			GetFileName() = 0;
			virtual bool			UnmapPage(BufferPage page) = 0;
			virtual BufferPage		AllocatePage() = 0;
			virtual bool			FreePage(BufferPage page) = 0;
			virtual void*			LockPage(BufferPage page) = 0;
			virtual bool			UnlockPage(BufferPage page, void* address, bool persist) = 0;
			virtual void			FillUnmapPageCandidates(collections::List<BufferPageTimeTuple>& pages, vint expectCount) = 0;
		};

		class BufferPageDesc
		{
		public:
			void*					address = nullptr;
			vuint64_t				offset = 0;
			bool					locked = false;
			vuint64_t				lastAccessTime = 0;
		};

		class BufferManager
		{
			typedef collections::Dictionary<BufferSource::IndexType, Ptr<IBufferSource>>	SourceMap;
		private:
			vuint64_t			pageSize;
			vuint64_t			cachePageCount;
			vuint64_t			pageSizeBits;
			volatile vuint64_t	totalCachedPages;
			SpinLock			lock;
			volatile vint		usedSourceIndex;
			SourceMap			sources;

			void				SwapCacheIfNecessary();
		public:
			BufferManager(vuint64_t _pageSize, vuint64_t _cachePageCount);
			~BufferManager();

			vuint64_t			GetPageSize();
			vuint64_t			GetCachePageCount();
			vuint64_t			GetCacheSize();
			vuint64_t			GetCurrentlyCachedPageCount();

			BufferSource		LoadMemorySource();
			BufferSource		LoadFileSource(const WString& fileName, bool createNew);
			bool				UnloadSource(BufferSource source);
			WString				GetSourceFileName(BufferSource source);

			void*				LockPage(BufferSource source, BufferPage page);
			bool				UnlockPage(BufferSource source, BufferPage page, void* buffer, bool persist);
			BufferPage			AllocatePage(BufferSource source);
			bool				FreePage(BufferSource source, BufferPage page);
			bool				EncodePointer(BufferPointer& pointer, BufferPage page, vuint64_t offset);
			bool				DecodePointer(BufferPointer pointer, BufferPage& page, vuint64_t& offset);
		};
	}
}

#endif
