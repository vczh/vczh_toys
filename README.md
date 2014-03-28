vczh_toys
=========

This repository contains vczh's toys for black magic codes or experiments

###./CppGarbageCollection
######// garbage collection library for C++
```c++
#include "gc_ptr.h"

using namespace vczh;

class Node : ENABLE_GC
{
public:
    gc_ptr<Node> next;
};

int main()
{
    gc_start(0x00100000, 0x00500000);
    {
        auto x = make_gc<Node>();
        auto y = make_gc<Node>();
        x->next = y;
        y->next = x;
        // static_gc_cast and dynamic_gc_cast is waiting for you who like doing pointer conversion
    }
    gc_force_collect(); // will search and delete x and y here
    gc_stop(); // will call gc_force_collect()
}

```

###./CppMultiDimentionArray
######// multiple dimentional array for C++
```c++
#include "array.h"

using namespace vczh;

int main()
{
	// int x[3][4][5]; but the lengths can be non-constant expressions
	array<int, 3> x((array_sizes(), 3, 4, 5));
	// first way to do x[1][2][3] = 10;
	x[1][2][3] = 10;
	// second way to do x[1][2][3] = 10;
	array_ref<int, 2> x1 = x[1];
	array_ref<int, 1> x2 = x1[2];
	int& x3 = x2[3];
	x3 = 10;
    return 0;
}
```