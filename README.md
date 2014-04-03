vczh_toys
=========

This repository contains vczh's toys for black magic codes or experiments:
* garbage collection library for C++11
* multiple dimentional array for C++11
* linq for C++11 and STL
* Y function for writing recursive lambda expression

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

###./CppLinq
######// linq for C++11 and STL
```c++
#include "linq.h"
#include <iostream>

using namespace std;
using namespace vczh;

int main()
{
	{
		// calculate sum of squares of odd numbers
		int xs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		int sum = from(xs)
			.where([](int x){return x % 2 == 1; })
			.select([](int x){return x * x; })
			.sum();
		// prints 165
		cout << sum << endl;
	}
	{
		// iterate of squares of odd numbers ordered by the last digit
		vector<int> xs = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		for (auto x : from(xs)
			.where([](int x){return x % 2 == 1; })
			.select([](int x){return x * x; })
			.order_by([](int x){return x % 10; })
			)
		{
			cout << x << " ";
		}
		// prints 1 81 25 9 49
		cout << endl;
	}
	{
		struct person
		{
			string		name;
		};

		struct pet
		{
			string		name;
			person		owner;
		};

		person magnus = { "Hedlund, Magnus" };
		person terry = { "Adams, Terry" };
		person charlotte = { "Weiss, Charlotte" };
		person persons[] = { magnus, terry, charlotte };

		pet barley = { "Barley", terry };
		pet boots = { "Boots", terry };
		pet whiskers = { "Whiskers", charlotte };
		pet daisy = { "Daisy", magnus };
		pet pets[] = { barley, boots, whiskers, daisy };

		auto person_name = [](const person& p){return p.name; };
		auto pet_name = [](const pet& p){return p.name; };
		auto pet_owner_name = [](const pet& p){return p.owner.name; };

		// print people and their animals in to levels
		/* prints
			Adams, Terry
				Barley
				Boots
			Hedlund, Magnus
				Daisy
			Weiss, Charlotte
				Whiskers
		*/
		for (auto x : from(persons).group_join(from(pets), person_name, pet_owner_name))
		{
			// x :: zip_pair<string, zip_pair<person, linq<pet>>>
			cout << x.second.first.name << endl;
			for (auto y : x.second.second)
			{
				cout << "    " << y.name << endl;
			}
		}

		// print people and their animals
		/* prints
			Adams, Terry: Barley
			Adams, Terry: Boots
			Hedlund, Magnus: Daisy
			Weiss, Charlotte: Whiskers
		*/
		for (auto x : from(persons).join(from(pets), person_name, pet_owner_name))
		{
			// x :: zip_pair<string, zip_pair<person, pet>>
			cout << x.second.first.name << ": " << x.second.second.name << endl;
		}
	}
}
```

###./CppRecursiveLambda
######// Y function for writing recursive lambda expression
```c++
#include "lambda.h"
#include <iostream>
#include <string>

using namespace std;
using namespace vczh;

int main()
{
	auto fib = Y([](function<int(int)> fib, int i)
	{
		return i < 2 ? 1 : fib(i - 1) + fib(i - 2);
	});

	for (int i = 0; i < 10; i++)
	{
		cout << fib(i) << ' ';
	}
	cout << endl;
}
```