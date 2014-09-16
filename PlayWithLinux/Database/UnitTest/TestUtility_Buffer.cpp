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
