#ifndef VCZH_DATABASE_UNITTEST
#define VCZH_DATABASE_UNITTEST

#include "../Source/DatabaseVlppReferences.h"

#define TEST_CASE(NAME)										\
	void TestFunction_##NAME(void);							\
	class TestClass_##NAME									\
	{														\
	public:													\
		TestClass_##NAME()									\
		{													\
			vl::console::Console::WriteLine(L ## #NAME);	\
			TestFunction_##NAME();							\
		}													\
	} TestInstance_##NAME;									\
	void TestFunction_##NAME(void)							\

#define TEST_ASSERT(EXPR)									\
	do{														\
		vl::console::Console::WriteLine(L"    " L ## #EXPR);\
		if (EXPR)											\
		{													\
			vl::console::Console::WriteLine(L"    [PASS]");	\
		}													\
		else												\
		{													\
			vl::console::Console::WriteLine(L"    [FAIL]");	\
			throw 0;										\
		}													\
	}while(0)												\

#endif
