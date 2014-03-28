#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include "linq.h"
#include <assert.h>
#include <vector>

using namespace std;
using namespace vczh;

int main()
{
	{
		vector<int> xs = { 1, 2, 3, 4, 5 };
		int sum = 0;
		for (auto x : from(xs.begin(), xs.end()))
		{
			sum += x;
		}
		assert(sum == 15);
	}
	{
		vector<int> xs = { 1, 2, 3, 4, 5 };
		int sum = 0;
		for (auto x : from(xs))
		{
			sum += x;
		}
		assert(sum == 15);
	}
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int sum = 0;
		for (auto x : from(from(from(xs))))
		{
			sum += x;
		}
		assert(sum == 15);
	}
	_CrtDumpMemoryLeaks();
	return 0;
}

