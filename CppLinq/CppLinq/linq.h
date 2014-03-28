#pragma once

#include <xutility>

namespace vczh
{
	//////////////////////////////////////////////////////////////////
	// select
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// where
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// linq
	//////////////////////////////////////////////////////////////////

	template<typename TIterator>
	class linq_enumerable
	{
	public:
		TIterator				_begin;
		TIterator				_end;

		linq_enumerable()
		{
		}

		linq_enumerable(const TIterator& _begin_, const TIterator& _end_)
			:_begin(_begin_), _end(_end_)
		{
		}

		TIterator begin()const
		{
			return _begin;
		}

		TIterator end()const
		{
			return _end;
		}
	};

	template<typename TIterator>
	linq_enumerable<TIterator> from(const TIterator& begin, const TIterator& end)
	{
		return linq_enumerable<TIterator>(begin, end);
	}

	template<typename TContainer>
	auto from(const TContainer& container)->linq_enumerable<decltype(std::begin(container))>
	{
		return linq_enumerable<decltype(std::begin(container))>(std::begin(container), std::end(container));
	}
}