#include "UnitTest.h"
#include "../Source/Utility/Buffer.h"

using namespace vl;
using namespace vl::database;

extern WString GetTempFolder();
#define TEMP_DIR GetTempFolder()+
#define KB *1024
#define MB *1024*1024

TEST_CASE(Utility_Buffer_AddRemoveSource)
{
	BufferManager bm(64 KB, 1 MB);
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
	BufferManager bm(64 KB, 1 MB);													\
	auto source = bm.LoadMemorySource();											\
	TestCase_Utility_Buffer_##NAME(bm, source);										\
}																					\
TEST_CASE(Utility_Buffer_File_##NAME)												\
{																					\
	BufferManager bm(64 KB, 1 MB);													\
	auto source = bm.LoadFileSource(TEMP_DIR L"db.bin", true);						\
	TestCase_Utility_Buffer_##NAME(bm, source);										\
}																					\
void TestCase_Utility_Buffer_##NAME(BufferManager& bm, BufferSource source)			\

TEST_CASE_SOURCE(AllocateFreePage)
{
}

TEST_CASE_SOURCE(LockUnlockPage)
{
}
