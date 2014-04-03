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