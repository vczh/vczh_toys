#include "UnitTest.h"
#include "../Source/Utility/Buffer.h"

using namespace vl;
using namespace vl::database;
using namespace vl::collections;

extern WString GetTempFolder();
#define TEMP_DIR GetTempFolder()+
#define KB *1024
#define MB *1024*1024

TEST_CASE(Utility_Buffer_AddRemoveSource)
{
	BufferManager bm(64 KB, 16);
	TEST_ASSERT(bm.GetPageSize() == 64 KB);
	TEST_ASSERT(bm.GetCacheSize() == 1 MB);
	TEST_ASSERT(bm.GetCachePageCount() == 16);

	auto a = bm.LoadMemorySource();
	auto b = bm.LoadFileSource(TEMP_DIR L"db.bin", true);
	TEST_ASSERT(bm.GetSourceFileName(a) == L"");
	TEST_ASSERT(bm.GetSourceFileName(b) == TEMP_DIR L"db.bin");

	bm.UnloadSource(a);
	bm.UnloadSource(b);
	TEST_ASSERT(bm.GetSourceFileName(a) == L"");
	TEST_ASSERT(bm.GetSourceFileName(b) == L"");

	a = bm.LoadMemorySource();
	b = bm.LoadFileSource(TEMP_DIR L"db.bin", true);
	TEST_ASSERT(bm.GetSourceFileName(a) == L"");
	TEST_ASSERT(bm.GetSourceFileName(b) == TEMP_DIR L"db.bin");
}

#define TEST_CASE_SOURCE(NAME)														\
extern void TestCase_Utility_Buffer_##NAME(BufferManager& bm, BufferSource source);	\
TEST_CASE(Utility_Buffer_InMemory_##NAME)											\
{																					\
	BufferManager bm(64 KB, 16);													\
	auto source = bm.LoadMemorySource();											\
	TestCase_Utility_Buffer_##NAME(bm, source);										\
}																					\
TEST_CASE(Utility_Buffer_File_##NAME)												\
{																					\
	BufferManager bm(64 KB, 16);													\
	auto source = bm.LoadFileSource(TEMP_DIR L"db.bin", true);						\
	TestCase_Utility_Buffer_##NAME(bm, source);										\
}																					\
void TestCase_Utility_Buffer_##NAME(BufferManager& bm, BufferSource source)			\

TEST_CASE_SOURCE(LockUnlockPage)
{
	auto page = bm.AllocatePage(source);
	TEST_ASSERT(page.IsValid());

	auto addr = bm.LockPage(source, page);
	TEST_ASSERT(addr != nullptr);
	TEST_ASSERT(bm.LockPage(source, page) == nullptr);

	TEST_ASSERT(bm.FreePage(source, page) == false);
	TEST_ASSERT(bm.UnlockPage(source, page, (char*)addr + 1, false) == false);
	TEST_ASSERT(bm.UnlockPage(source, page, addr, false) == true);
	TEST_ASSERT(bm.UnlockPage(source, page, addr, false) == false);

	TEST_ASSERT(bm.FreePage(source, page) == true);
	TEST_ASSERT(bm.UnlockPage(source, page, (char*)addr + 1, false) == false);
	TEST_ASSERT(bm.UnlockPage(source, page, addr, false) == false);
	TEST_ASSERT(bm.UnlockPage(source, page, addr, false) == false);
}

TEST_CASE_SOURCE(AllocateFreePage)
{
	auto page1 = bm.AllocatePage(source);
	TEST_ASSERT(page1.IsValid());
	auto page2 = bm.AllocatePage(source);
	TEST_ASSERT(page2.IsValid());
	TEST_ASSERT(page1.index != page2.index);

	auto addr1 = bm.LockPage(source, page1);
	TEST_ASSERT(addr1 != nullptr);
	auto addr2 = bm.LockPage(source, page2);
	TEST_ASSERT(addr2 != nullptr);

	TEST_ASSERT(bm.UnlockPage(source, page1, addr1, false) == true);
	TEST_ASSERT(bm.FreePage(source, page1) == true);
	TEST_ASSERT(bm.LockPage(source, page1) == nullptr);

	strcpy((char*)addr2, "This is page 2");
	TEST_ASSERT(bm.UnlockPage(source, page2, addr2, true) == true);

	addr2 = bm.LockPage(source, page2);
	TEST_ASSERT(addr2 != nullptr);
	TEST_ASSERT(strcmp((char*)addr2, "This is page 2") == 0);
	TEST_ASSERT(bm.UnlockPage(source, page2, addr2, false) == true);

	auto page3 = bm.AllocatePage(source);
	TEST_ASSERT(page3.index == page1.index);
	auto addr3 = bm.LockPage(source, page3);
	TEST_ASSERT(addr3 != nullptr);

	strcpy((char*)addr3, "This is page 3");
	TEST_ASSERT(bm.UnlockPage(source, page3, addr3, true) == true);

	addr2 = bm.LockPage(source, page2);
	TEST_ASSERT(addr2 != nullptr);
	addr3 = bm.LockPage(source, page3);
	TEST_ASSERT(addr3 != nullptr);
	TEST_ASSERT(strcmp((char*)addr2, "This is page 2") == 0);
	TEST_ASSERT(strcmp((char*)addr3, "This is page 3") == 0);

	TEST_ASSERT(bm.LockPage(source, page2) == nullptr);
	TEST_ASSERT(bm.UnlockPage(source, page2, addr2, true) == true);
	TEST_ASSERT(bm.LockPage(source, page3) == nullptr);
	TEST_ASSERT(bm.UnlockPage(source, page3, addr3, true) == true);
}

TEST_CASE(Utility_Buffer_File_AllocateFreeManyTimes)
{
	BufferManager bm(4 KB, 16);
	auto source = bm.LoadFileSource(TEMP_DIR L"db.bin", true);
	List<BufferPage> pages;
	const vint TotalCount = 32768;

	for (vint i = 0; i < TotalCount; i++)
	{
		console::Console::WriteLine(L"Allocating " + itow(i) + L" ... ");

		auto page = bm.AllocatePage(source);
		if (!page.IsValid()) throw 0;
		pages.Add(page);
	}

	for (vint i = 0; i < TotalCount; i++)
	{
		console::Console::WriteLine(L"Freeing " + itow(i) + L" ... ");

		auto page = pages[i];
		if (!bm.FreePage(source, page)) throw 0;
	}

	for (vint i = 0; i < TotalCount; i++)
	{
		console::Console::WriteLine(L"Allocating " + itow(i) + L" ... ");

		auto page = bm.AllocatePage(source);
		if (!page.IsValid()) throw 0;
		if (page.index != pages[TotalCount - i - 1].index) throw 0;
	}
}
