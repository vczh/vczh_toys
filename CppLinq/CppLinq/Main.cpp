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
	//////////////////////////////////////////////////////////////////
	// from
	//////////////////////////////////////////////////////////////////
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
	//////////////////////////////////////////////////////////////////
	// select
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int sum = 0;
		for (auto x : from(xs).select([](int x){return x * 2; }))
		{
			sum += x;
		}
		assert(sum == 30);
	}
	//////////////////////////////////////////////////////////////////
	// hide type test
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int sum = 0;
		linq<int> hidden = from(xs).select([](int x){return x * 2; });
		for (auto x : hidden)
		{
			sum += x;
		}
		assert(sum == 30); // 1 + 2 + 3 + 4 + 5
	}
	//////////////////////////////////////////////////////////////////
	// where
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int sum = 0;
		for (auto x : from(xs).where([](int x){return x % 2 == 0; }))
		{
			sum += x;
		}
		assert(sum == 6); // 2 + 4
	}
	_CrtDumpMemoryLeaks();
	return 0;
}

