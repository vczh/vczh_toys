#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include <assert.h>
#include "linq.h"
#include <iostream>

using namespace std;
using namespace vczh;

int test();

struct person
{
	string		name;
};

struct pet
{
	string		name;
	person		owner;
};

int main()
{
	test();
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

int test()
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
		int sum = 0;
		for (auto x : from_values({ 1, 2, 3, 4, 5 }))
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
		assert(from(xs).select([](int x){return x * 2; }).sequence_equal({ 2, 4, 6, 8, 10 }));
	}
	//////////////////////////////////////////////////////////////////
	// hide type test
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		linq<int> hidden = from(xs).select([](int x){return x * 2; });
		assert(hidden.sequence_equal({ 2, 4, 6, 8, 10 }));
	}
	//////////////////////////////////////////////////////////////////
	// where
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		assert(from(xs).where([](int x){return x % 2 == 0; }).sequence_equal({ 2, 4 }));
	}
	//////////////////////////////////////////////////////////////////
	// iterating
	//////////////////////////////////////////////////////////////////
	{
		vector<int> empty;
		int xs[] = { 1, 2, 3, 4, 5 };
		int ys[] = { 1, 2, 3 };
		int zs[] = { 4, 5 };
		assert(from(xs).take(3).sequence_equal(ys));
		assert(from(xs).skip(3).sequence_equal(zs));
		assert(from(xs).take_while([](int a){return a != 4; }).sequence_equal(ys));
		assert(from(xs).skip_while([](int a){return a != 4; }).sequence_equal(zs));
		assert(from(xs).take(0).sequence_equal(empty));
		assert(from(xs).skip(5).sequence_equal(empty));
		assert(from(ys).concat(from(zs)).sequence_equal(xs));
		assert(from(xs).concat(from(empty)).sequence_equal(xs));
		assert(from(empty).concat(from(xs)).sequence_equal(xs));
		assert(from(empty).concat(from(empty)).sequence_equal(empty));
		assert(from(ys).concat(zs).sequence_equal(xs));
		assert(from(xs).concat(empty).sequence_equal(xs));
		assert(from(empty).concat(xs).sequence_equal(xs));
		assert(from(empty).concat(empty).sequence_equal(empty));
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

		assert(from(a).sequence_equal(b));
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

		assert(from(a).default_if_empty(0).sequence_equal(b));
		assert(from(c).default_if_empty(0).sequence_equal(g));

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

		assert(from(c).single_or_default(0).sequence_equal(g));
		assert(from(g).single().sequence_equal(g));
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
		assert(from(xs).sequence_equal(from(xs).to_vector()));
		assert(from(xs).sequence_equal(from(xs).to_deque()));
		assert(from(xs).sequence_equal(from(xs).to_list()));
		assert(from(xs).sequence_equal(from(xs).to_set()));

		auto f = [](int x){return x; };
		assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p){return p.first; })));
		assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p){return p.second; })));
		assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p){return p.first; })));
		assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p){return p.second; })));
	}
	//////////////////////////////////////////////////////////////////
	// aggregating
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
	//////////////////////////////////////////////////////////////////
	// set
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 1, 2, 2, 3, 3 };
		int ys[] = { 2, 2, 3, 3, 4, 4 };
		assert(from(xs).distinct().sequence_equal({ 1, 2, 3 }));
		assert(from(ys).distinct().sequence_equal({ 2, 3, 4 }));
		assert(from(xs).except_with(ys).sequence_equal({ 1 }));
		assert(from(xs).intersect_with(ys).sequence_equal({ 2, 3 }));
		assert(from(xs).union_with(ys).sequence_equal({ 1, 2, 3, 4 }));
	}
	//////////////////////////////////////////////////////////////////
	// restructuring
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 1, 2, 3, 4, 5 };
		int ys[] = { 6, 7, 8, 9, 10 };

		zip_pair<int, int> zs[] = { { 1, 6 }, { 2, 7 }, { 3, 8 }, { 4, 9 }, { 5, 10 } };
		assert(from(xs).zip_with(ys).sequence_equal(zs));

		auto g = from(xs).group_by([](int x){return x % 2; });
		assert(g.select([](zip_pair<int, linq<int>> p){return p.first; }).sequence_equal({ 0, 1 }));
		assert(g.first().second.sequence_equal({ 2, 4 }));
		assert(g.last().second.sequence_equal({ 1, 3, 5 }));

		assert(
			from_values({ 1, 2, 3 })
			.select_many([](int x){return from_values({ x, x*x, x*x*x }); })
			.sequence_equal({ 1, 1, 1, 2, 4, 8, 3, 9, 27 })
			);
	}
	//////////////////////////////////////////////////////////////////
	// ordering
	//////////////////////////////////////////////////////////////////
	{
		int xs[] = { 7, 1, 12, 2, 8, 3, 11, 4, 9, 5, 13, 6, 10 };
		int ys[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
		int zs[] = { 10, 1, 11, 2, 12, 3, 13, 4, 5, 6, 7, 8, 9 };

		assert(from(xs).order_by([](int x){return x; }).sequence_equal(ys));
		assert(
			flatten(
				from(xs)
				.first_order_by([](int x){return x % 10; })
				.then_order_by([](int x){return x / 10; })
				)
			.sequence_equal(zs)
			);
	}
	//////////////////////////////////////////////////////////////////
	// joining
	//////////////////////////////////////////////////////////////////
	{
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
		
		auto f = from(persons).full_join(from(pets), person_name, pet_owner_name);
		{
			typedef join_pair<string, linq<person>, linq<pet>> TItem;
			auto xs = f.to_vector();
			assert(from(xs).select([](const TItem& item){return item.first; }).sequence_equal({ terry.name, magnus.name, charlotte.name }));
			assert(xs[0].second.first.select(person_name).sequence_equal({ terry.name }));
			assert(xs[1].second.first.select(person_name).sequence_equal({ magnus.name }));
			assert(xs[2].second.first.select(person_name).sequence_equal({ charlotte.name }));
			assert(xs[0].second.second.select(pet_name).sequence_equal({ barley.name, boots.name }));
			assert(xs[1].second.second.select(pet_name).sequence_equal({ daisy.name }));
			assert(xs[2].second.second.select(pet_name).sequence_equal({ whiskers.name }));
		}
		auto g = from(persons).group_join(from(pets), person_name, pet_owner_name);
		{
			typedef join_pair<string, person, linq<pet>> TItem;
			auto xs = g.to_vector();
			assert(from(xs).select([](const TItem& item){return item.first; }).sequence_equal({ terry.name, magnus.name, charlotte.name }));
			assert(xs[0].second.first.name == terry.name);
			assert(xs[1].second.first.name == magnus.name);
			assert(xs[2].second.first.name == charlotte.name);
			assert(xs[0].second.second.select(pet_name).sequence_equal({ barley.name, boots.name }));
			assert(xs[1].second.second.select(pet_name).sequence_equal({ daisy.name }));
			assert(xs[2].second.second.select(pet_name).sequence_equal({ whiskers.name }));
		}
		auto j = from(persons).join(from(pets), person_name, pet_owner_name);
		{
			typedef join_pair<string, person, pet> TItem;
			auto xs = j.to_vector();
			assert(from(xs).select([](const TItem& item){return item.first; }).sequence_equal({ terry.name, terry.name, magnus.name, charlotte.name }));
			assert(xs[0].second.first.name == terry.name);
			assert(xs[1].second.first.name == terry.name);
			assert(xs[2].second.first.name == magnus.name);
			assert(xs[3].second.first.name == charlotte.name);
			assert(xs[0].second.second.name == barley.name);
			assert(xs[1].second.second.name == boots.name);
			assert(xs[2].second.second.name == daisy.name);
			assert(xs[3].second.second.name == whiskers.name);
		}
	}
#ifdef _MSC_VER
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

