#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include <assert.h>
#include "gc_ptr.h"
#include <iostream>
#include <string>

using namespace std;
using namespace vczh;

class A : ENABLE_GC
{
public:
	gc_ptr<A>		next;

	A(int a)
	{
	}

	~A()
	{
		assert(next.operator->() == nullptr);
	}
};

class B : ENABLE_GC, public virtual A
{
public:
	B(int a) :A(a)
	{
	}

	~B()
	{
		assert(next.operator->() == nullptr);
	}
};

class C : ENABLE_GC, public virtual A
{
public:
	C(int a) :A(a)
	{
	}

	~C()
	{
		assert(next.operator->() == nullptr);
	}
};

class D : ENABLE_GC, public B, public C
{
public:
	D(int a) :A(a), B(a), C(a)
	{
	}

	~D()
	{
		assert(next.operator->() == nullptr);
	}
};

int main()
{
	int step_size = 1024;		// collect whenever the increment of the memory exceeds <step_size> bytes
	int max_size = 8192;		// collect whenever the total memory used exceeds <max_size> bytes
	gc_start(step_size, max_size);
	for (int i = 0; i < 65536; i++)
	{
		auto x = make_gc<B>(1);
		auto y = make_gc<C>(2);
		auto z = make_gc<D>(3);
		x->next = y;
		y->next = z;
		z->next = x;

		assert(dynamic_gc_cast<C>(x->next));
		assert(dynamic_gc_cast<D>(y->next));
		assert(dynamic_gc_cast<B>(z->next));
		if (i % 1000 == 0)
		{
			cout << i << endl;
		}
	}
	gc_stop();
#ifdef _MSC_VER
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
