#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include "linq.h"
#include <assert.h>

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
		int ys[] = { 2, 4, 6, 8, 10 };
		assert(from(xs).select([](int x){return x * 2; }).sequence_equal(from(ys)));
	}
	//////////////////////////////////////////////////////////////////
	// hide type test
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int ys[] = { 2, 4, 6, 8, 10 };
		linq<int> hidden = from(xs).select([](int x){return x * 2; });
		assert(hidden.sequence_equal(from(ys)));
	}
	//////////////////////////////////////////////////////////////////
	// where
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int ys[] = { 2, 4 };
		assert(from(xs).where([](int x){return x % 2 == 0; }).sequence_equal(from(ys)));
	}
	//////////////////////////////////////////////////////////////////
	// counting
	//////////////////////////////////////////////////////////////////
	{
		int a[] = { 1, 2, 3, 4, 5 };
		vector<int> b = { 1, 2, 3, 4, 5 };
		vector<int> c;
		int d[] = { 1, 2, 3, 4 };
		int e[] = { 1, 2, 3, 4, 5, 6 };
		int f[] = { 6, 7, 8, 9, 10 };
		int g[] = { 0 };
		linq<int> xs[] = { from(b), from(c), from(d), from(e), from(f) };

		assert(from(a).sequence_equal(from(b)));
		for (auto& x : xs)
		{
			for (auto& y : xs)
			{
				assert(x.sequence_equal(y) == (&x == &y));
			}
		}

		assert(from(a).contains(1));
		assert(from(a).contains(5));
		assert(!from(a).contains(6));
		assert(!from(c).contains(6));

		assert(from(a).count() == 5);
		assert(from(c).count() == 0);

		assert(from(a).default_if_empty(0).sequence_equal(from(b)));
		assert(from(c).default_if_empty(0).sequence_equal(from(g)));

		assert(from(a).element_at(0) == 1);
		assert(from(a).element_at(4) == 5);
		try{ from(a).element_at(-1); assert(false); }
		catch (const linq_exception&){}
		try{ from(a).element_at(6); assert(false); }
		catch (const linq_exception&){}
		try{ from(c).element_at(0); assert(false); }
		catch (const linq_exception&){}

		assert(!from(a).empty());
		assert(from(c).empty());

		assert(from(a).first() == 1);
		assert(from(a).first_or_default(0) == 1);
		assert(from(a).last() == 5);
		assert(from(a).last_or_default(0) == 5);
		assert(from(c).first_or_default(0) == 0);
		assert(from(c).last_or_default(0) == 0);
		try{ from(c).first(); assert(false); }
		catch (const linq_exception&){}
		try{ from(c).last(); assert(false); }
		catch (const linq_exception&){}

		assert(from(c).single_or_default(0).sequence_equal(from(g)));
		assert(from(g).single().sequence_equal(from(g)));
		try{ from(a).single(); assert(false); }
		catch (const linq_exception&){}
		try{ from(a).single_or_default(0); assert(false); }
		catch (const linq_exception&){}
		try{ from(c).single(); assert(false); }
		catch (const linq_exception&){}
	}
	//////////////////////////////////////////////////////////////////
	// containers
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		assert(from(xs).sequence_equal(from(from(xs).to_vector())));
		assert(from(xs).sequence_equal(from(from(xs).to_deque())));
		assert(from(xs).sequence_equal(from(from(xs).to_list())));
		assert(from(xs).sequence_equal(from(from(xs).to_set())));
		assert(from(xs).sequence_equal(from(from(xs).to_multiset())));
		assert(from(xs).sequence_equal(from(from(xs).to_unordered_set())));

		auto f = [](int x){return x; };
		assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p){return p.first; })));
		assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p){return p.second; })));
		assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p){return p.first; })));
		assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p){return p.second; })));
		assert(from(xs).sequence_equal(from(from(xs).to_unordered_map(f)).select([](pair<int, int> p){return p.first; })));
		assert(from(xs).sequence_equal(from(from(xs).to_unordered_map(f)).select([](pair<int, int> p){return p.second; })));
	}
	//////////////////////////////////////////////////////////////////
	// aggregation
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		assert(from(xs).aggregate([](int a, int b){return a + b; }) == 15);
		assert(from(xs).aggregate(0, [](int a, int b){return a + b; }) == 15);
		assert(from(xs).sum() == 15);
		assert(from(xs).aggregate([](int a, int b){return a * b; }) == 120);
		assert(from(xs).aggregate(1, [](int a, int b){return a * b; }) == 120);
		assert(from(xs).product() == 120);
		assert(from(xs).all([](int a){return a > 1; }) == false);
		assert(from(xs).all([](int a){return a > 0; }) == true);
		assert(from(xs).any([](int a){return a > 1; }) == true);
		assert(from(xs).any([](int a){return a > 0; }) == true);
		assert(from(xs).min() == 1);
		assert(from(xs).max() == 5);
		assert(from(xs).average<double>() == 3.0);

		vector<int> ys;
		try{ from(ys).product(); assert(false); }
		catch (const linq_exception&){}
		try{ from(ys).min(); assert(false); }
		catch (const linq_exception&){}
		try{ from(ys).max(); assert(false); }
		catch (const linq_exception&){}
		try{ from(ys).average<int>(); assert(false); }
		catch (const linq_exception&){}
	}
	_CrtDumpMemoryLeaks();
	return 0;
}

