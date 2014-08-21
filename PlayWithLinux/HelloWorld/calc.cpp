#include <string.h>
#include "calc.h"

using It = string::const_iterator;

char test_char(It& i, It end, const char* candidates)
{
	if (i == end) return 0;
	auto result = strchr(candidates, *i);
	if (!result) return 0;
	i++;
	return *result;
}

bool number(It& i, It end, int& result)
{
	result = 0;
	It j = i;
	while(j != end && '0' <= *j && *j <= '9')
	{
		result = result * 10 + (*j - '0');
		j++;
	}
	if (i == j) return false;
	i = j;
	return true;
}

bool factor(It& i, It end, int& result);
bool term(It& i, It end, int& result);
bool exp(It& i, It end, int& result);

bool factor(It& i, It end, int& result)
{
	if (number(i, end, result)) return true;
	auto j = i;
	if (!test_char(j, end, "(")) return false;
	if (!exp(j, end, result)) return false;
	if (!test_char(j, end, ")")) return false;
	i = j;
	return true;
}

bool term(It& i, It end, int& result)
{
	if (!factor(i, end, result)) return false;
	while (true)
	{
		auto j = i;
		if (auto op = test_char(j, end, "*/"))
		{
			int next = 0;
			if(!factor(j, end, next)) return true;
			result = op == '*' 
				? result * next
				: result / next;
			i = j;
		}
		else
		{
			return true;
		}
	}
}

bool exp(It& i, It end, int& result)
{
	if (!term(i, end, result)) return false;
	while (true)
	{
		auto j = i;
		if (auto op = test_char(j, end, "+-"))
		{
			int next = 0;
			if(!term(j, end, next)) return true;
			result = op == '+' 
				? result + next
				: result - next;
			i = j;
		}
		else
		{
			return true;
		}
	}
}


bool calc(const string& input, int& result)
{
	auto begin = input.begin();
	auto end = input.end();
	return exp(begin, end, result);
}
