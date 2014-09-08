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


#endif
