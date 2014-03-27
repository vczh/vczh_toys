vczh_toys
=========

This repository contains vczh's toys for black magic codes or experiments

### ./CppGarbageCollection // garbage collection library for C++
```c++
#include "gc_cpp.h"

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
