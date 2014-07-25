#include <iostream>

using namespace std;

/*
prime = sieve [2..] where
sieve (x:xs) = x : sieve (filter (\y ->y `rem` x /= 0) xs)
*/

template<int Number, typename T>
struct List
{
	static const int Data = Number;
	typedef T Next;
};

struct ListEnd
{
};

////////////////////////////////////

template<typename T, int Divider>
struct FilterOutDivisable
{
	typedef void Result;
};

template<int Number, typename T, int Divider>
struct FilterOutDivisable<List<Number, T>, Divider>
{
	template<bool Divisable>
	struct Internal
	{
	};

	template<>
	struct Internal<true>
	{
		typedef typename FilterOutDivisable<T, Divider>::Result Result;
	};

	template<>
	struct Internal<false>
	{
		typedef List<Number, typename FilterOutDivisable<T, Divider>::Result> Result;
	};

	typedef typename Internal<Number%Divider == 0>::Result Result;
};

template<int Divider>
struct FilterOutDivisable<ListEnd, Divider>
{
	typedef ListEnd Result;
};

////////////////////////////////////

template<typename T>
struct Sieve
{
	typedef void Result;
};

template<int Number, typename T>
struct Sieve<List<Number, T>>
{
	typedef typename List<Number, typename Sieve<typename FilterOutDivisable<T, Number>::Result>::Result> Result;
};

template<>
struct Sieve<ListEnd>
{
	typedef ListEnd Result;
};

////////////////////////////////////

template<typename T, int Counter>
struct Count
{
	static const int Result = -1;
};

template<int Number, typename T>
struct Count<List<Number, T>, 1>
{
	static const int Result = Number;
};

template<int Number, typename T, int Counter>
struct Count<List<Number, T>, Counter>
{
	static const int Result = Count<T, Counter - 1>::Result;
};

////////////////////////////////////

template<int First, int Last>
struct Parameter
{
	typedef typename List<First, typename Parameter<First + 1, Last>::Result> Result;
};

template<int Last>
struct Parameter<Last, Last>
{
	typedef ListEnd Result;
};

struct Primes
{
	typedef Sieve<Parameter<2, 80000000>::Result>::Result Result;
};

////////////////////////////////////

int main()
{
	cout << Count<Primes::Result, 4263116>::Result << endl;
	return 0;
}