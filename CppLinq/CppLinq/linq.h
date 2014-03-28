#pragma once

#include <xutility>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

namespace vczh
{
	template<typename TIterator>
	using iterator_type = decltype(**(TIterator*)0);

	namespace iterators
	{
		//////////////////////////////////////////////////////////////////
		// hide_type
		//////////////////////////////////////////////////////////////////

		template<typename T>
		class hide_type_iterator
		{
		private:
			class iterator_interface
			{
				typedef iterator_interface							TSelf;
			public:
				virtual std::shared_ptr<TSelf>	next() = 0;
				virtual T						deref() = 0;
				virtual bool					equals(const std::shared_ptr<TSelf>& it) = 0;
			};

			template<typename TIterator>
			class iterator_implement : public iterator_interface
			{
				typedef iterator_implement<TIterator>				TSelf;
			private:
				TIterator						iterator;

			public:
				iterator_implement(const TIterator& _iterator)
					:iterator(_iterator)
				{
				}

				std::shared_ptr<iterator_interface> next()override
				{
					TIterator t = iterator;
					t++;
					return std::make_shared<TSelf>(t);
				}

				T deref()override
				{
					return *iterator;
				}

				bool equals(const std::shared_ptr<iterator_interface>& it)override
				{
					auto impl = std::dynamic_pointer_cast<TSelf>(it);
					return impl && (iterator == impl->iterator);
				}
			};
			
			typedef hide_type_iterator<T>							TSelf;

			std::shared_ptr<iterator_interface>		iterator;
		public:
			template<typename TIterator>
			hide_type_iterator(const TIterator& _iterator)
				:iterator(std::make_shared<iterator_implement<TIterator>>(_iterator))
			{
			}

			TSelf& operator++()
			{
				iterator = iterator->next();
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				iterator = iterator->next();
				return t;
			}

			T operator*()const
			{
				return iterator->deref();
			}

			bool operator==(const TSelf& it)const
			{
				return iterator->equals(it.iterator);
			}

			bool operator!=(const TSelf& it)const
			{
				return !(iterator->equals(it.iterator));
			}
		};

		//////////////////////////////////////////////////////////////////
		// select :: [T] -> (T -> U) -> [U]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator, typename TFunction>
		class select_iterator
		{
			typedef select_iterator<TIterator, TFunction>			TSelf;
		private:
			TIterator			iterator;
			TFunction			f;

		public:
			select_iterator(const TIterator& _iterator, const TFunction& _f)
				:iterator(_iterator), f(_f)
			{
			}

			TSelf& operator++()
			{
				iterator++;
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				iterator++;
				return t;
			}

			auto operator*()const->decltype(f(*iterator))
			{
				return f(*iterator);
			}

			bool operator==(const TSelf& it)const
			{
				return iterator == it.iterator;
			}

			bool operator!=(const TSelf& it)const
			{
				return iterator != it.iterator;
			}
		};

		//////////////////////////////////////////////////////////////////
		// where :: [T] -> (T -> bool) -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator, typename TFunction>
		class where_iterator
		{
			typedef where_iterator<TIterator, TFunction>			TSelf;
		private:
			TIterator			iterator;
			TIterator			end;
			TFunction			f;

			void move_iterator(bool next)
			{
				if (iterator == end) return;
				if (next) iterator++;
				while (iterator != end && !f(*iterator))
				{
					iterator++;
				}
			}
		public:
			where_iterator(const TIterator& _iterator, const TIterator& _end, const TFunction& _f)
				:iterator(_iterator), end(_end), f(_f)
			{
				move_iterator(false);
			}

			TSelf& operator++()
			{
				move_iterator(true);
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				move_iterator(true);
				return t;
			}

			iterator_type<TIterator> operator*()const
			{
				return *iterator;
			}

			bool operator==(const TSelf& it)const
			{
				return iterator == it.iterator;
			}

			bool operator!=(const TSelf& it)const
			{
				return iterator != it.iterator;
			}
		};

		//////////////////////////////////////////////////////////////////
		// single
		//////////////////////////////////////////////////////////////////

		template<typename T>
		class single_iterator
		{
			typedef single_iterator<T>								TSelf;
		private:
			T					value;
			bool				end;

		public:
			single_iterator(const T& _value, bool _end)
				:value(_value), end(_end)
			{
			}

			TSelf& operator++()
			{
				end = true;
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				end = true;
				return t;
			}

			T operator*()const
			{
				if (end)
				{
					throw linq_exception("Failed to get the value from an iterator which is out of range.");
				}
				return value;
			}

			bool operator==(const TSelf& it)const
			{
				return end == it.end;
			}

			bool operator!=(const TSelf& it)const
			{
				return end != it.end;
			}
		};
	}

	template<typename TIterator>
	class linq_enumerable;

	namespace types
	{
		template<typename TIterator, typename TFunction>
		using select_it = iterators::select_iterator<TIterator, TFunction>;

		template<typename TIterator, typename TFunction>
		using where_it = iterators::where_iterator<TIterator, TFunction>;

		template<typename T>
		using single_it = iterators::single_iterator<T>;
	}

	//////////////////////////////////////////////////////////////////
	// linq
	//////////////////////////////////////////////////////////////////

	class linq_exception
	{
	public:
		std::string				message;

		linq_exception(const std::string& _message)
			:message(_message)
		{
		}
	};

	template<typename T>
	class linq;

	template<typename TIterator>
	class linq_enumerable
	{
		typedef typename std::remove_cv<typename std::remove_reference<iterator_type<TIterator>>::type>::type	TElement;
	private:
		TIterator				_begin;
		TIterator				_end;

	public:
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

		//////////////////////////////////////////////////////////////////
		// iterating
		//////////////////////////////////////////////////////////////////

		template<typename TFunction>
		linq_enumerable<types::select_it<TIterator, TFunction>> select(const TFunction& f)const
		{
			return linq_enumerable<types::select_it<TIterator, TFunction>>(
				types::select_it<TIterator, TFunction>(_begin, f),
				types::select_it<TIterator, TFunction>(_end, f)
				);
		}

		template<typename TFunction>
		linq_enumerable<types::where_it<TIterator, TFunction>> where(const TFunction& f)const
		{
			return linq_enumerable<types::where_it<TIterator, TFunction>>(
				types::where_it<TIterator, TFunction>(_begin, _end, f),
				types::where_it<TIterator, TFunction>(_end, _end, f)
				);
		}

		void concat()const;
		void cast()const;
		void typeof()const;
		void skip()const;
		void skip_while()const;
		void sum()const;
		void then_by()const;

		//////////////////////////////////////////////////////////////////
		// counting
		//////////////////////////////////////////////////////////////////

		template<typename T>
		bool contains(const T& t)const
		{
			for (auto it = _begin; it != _end; it++)
			{
				if (*it == t) return true;
			}
			return false;
		}

		int count()const
		{
			int counter = 0;
			for (auto it = _begin; it != _end; it++)
			{
				counter++;
			}
			return counter;
		}

		linq<TElement> default_if_empty(const TElement& value)const
		{
			if (count() == 0)
			{
				return linq_enumerable<types::single_it<TElement>>(
					types::single_it<TElement>(value, false),
					types::single_it<TElement>(value, true)
					);
			}
			else
			{
				return *this;
			}
		}

		TElement element_at(int index)const
		{
			if (index >= 0)
			{
				int counter = 0;
				for (auto it = _begin; it != _end; it++)
				{
					if (counter == index) return *it;
					counter++;
				}
			}
			throw linq_exception("Argument out of range: index.");
		}

		bool empty()const
		{
			return _begin == _end;
		}

		TElement first()const
		{
			if (empty()) throw linq_exception("Failed to get a value from an empty collection.");
			return *_begin;
		}

		TElement first_or_default(const TElement& value)const
		{
			return empty() ? value : *_begin;
		}

		TElement last()const
		{
			if (empty()) throw linq_exception("Failed to get a value from an empty collection.");
			auto it = _begin;
			TElement result = *it;
			while (++it != _end)
			{
				result = *it;
			}
			return result;
		}

		TElement last_or_default(const TElement& value)const
		{
			auto result = value;
			for (auto it = _begin; it != _end; it++)
			{
				result = *it;
			}
			return result;
		}

		linq_enumerable<TIterator> single()const
		{
			auto it = _begin;
			if (it == _end) throw linq_exception("Failed to get a value from an empty collection.");

			it++;
			if (it!=_end) throw linq_exception("The collection should have exactly one value.");

			return *this;
		}

		linq<TElement> single_or_default(const TElement& value)const
		{
			auto it = _begin;
			if (it == _end) return linq_enumerable<types::single_it<TElement>>(
				types::single_it<TElement>(value, false),
				types::single_it<TElement>(value, true)
				);

			it++;
			if (it!=_end) throw linq_exception("The collection should have exactly one value.");

			return *this;
		}
		
		template<typename TIterator2>
		bool sequence_equal(const linq_enumerable<TIterator2>& e)const
		{
			auto x = _begin;
			auto xe = _end;
			auto y = e.begin();
			auto ye = e.end();

			while (x != xe && y != ye)
			{
				if (*x++ != *y++) return false;
			}
			return x == xe && y == ye;
		}

		//////////////////////////////////////////////////////////////////
		// set
		//////////////////////////////////////////////////////////////////

		void distinct_with()const;
		void except_with()const;
		void intersect_with()const;
		void union_with()const;

		//////////////////////////////////////////////////////////////////
		// aggregation
		//////////////////////////////////////////////////////////////////

		template<typename TFunction>
		TElement aggregate(const TFunction& f)const
		{
			auto it = _begin;
			if (it == _end) throw linq_exception("Failed to get a value from an empty collection.");

			TElement result = *it;
			while (++it != _end)
			{
				result = f(result, *it);
			}
			return result;
		}

		template<typename TResult, typename TFunction>
		TResult aggregate(const TResult& init, const TFunction& f)
		{
			TResult result = init;
			for (auto it = _begin; it != _end; it++)
			{
				result = f(result, *it);
			}
			return result;
		}

		template<typename TFunction>
		bool all(const TFunction& f)const
		{
			return select(f).aggregate(true, [](bool a, bool b){return a&&b; });
		}
		
		template<typename TFunction>
		bool any(const TFunction& f)const
		{
			return !where(f).empty();
		}

		template<typename TResult>
		TResult average()const
		{
			if (_begin == _end) throw linq_exception("Failed to get a value from an empty collection.");
			TResult sum = 0;
			int counter = 0;
			for (auto it = _begin; it != _end; it++)
			{
				sum += (TResult)*it;
				counter++;
			}
			return sum / counter;
		}

		TElement max()const
		{
			return aggregate([](const TElement& a, const TElement& b){return a > b ? a : b; });
		}

		TElement min()const
		{
			return aggregate([](const TElement& a, const TElement& b){return a < b ? a : b; });
		}

		TElement sum()
		{
			return aggregate(0, [](const TElement& a, const TElement& b){return a + b; });
		}

		TElement product()
		{
			return aggregate([](const TElement& a, const TElement& b){return a * b; });
		}

		//////////////////////////////////////////////////////////////////
		// restructuring
		//////////////////////////////////////////////////////////////////

		void group_by()const;
		void group_join()const;
		void join()const;
		void order_by()const;
		void zip()const;

		//////////////////////////////////////////////////////////////////
		// containers
		//////////////////////////////////////////////////////////////////

		std::vector<TElement> to_vector()const
		{
			std::vector<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.push_back(*it);
			}
			return std::move(container);
		}

		std::list<TElement> to_list()const
		{
			std::list<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.push_back(*it);
			}
			return std::move(container);
		}

		std::deque<TElement> to_deque()const
		{
			std::deque<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.push_back(*it);
			}
			return std::move(container);
		}

		template<typename TFunction>
		auto to_map(const TFunction& keySelector)const->std::map<decltype(keySelector(*(TElement*)0)), TElement>
		{
			std::map<decltype(keySelector(*(TElement*)0)), TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(std::make_pair(keySelector(*it), *it));
			}
			return std::move(container);
		}

		template<typename TFunction>
		auto to_multimap(const TFunction& keySelector)const->std::multimap<decltype(keySelector(*(TElement*)0)), TElement>
		{
			std::multimap<decltype(keySelector(*(TElement*)0)), TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(std::make_pair(keySelector(*it), *it));
			}
			return std::move(container);
		}

		template<typename TFunction>
		auto to_unordered_map(const TFunction& keySelector)const->std::unordered_map<decltype(keySelector(*(TElement*)0)), TElement>
		{
			std::unordered_map<decltype(keySelector(*(TElement*)0)), TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(std::make_pair(keySelector(*it), *it));
			}
			return std::move(container);
		}

		std::set<TElement> to_set()const
		{
			std::set<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(*it);
			}
			return std::move(container);
		}

		std::multiset<TElement> to_multiset()const
		{
			std::multiset<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(*it);
			}
			return std::move(container);
		}

		std::unordered_set<TElement> to_unordered_set()const
		{
			std::unordered_set<TElement> container;
			for (auto it = _begin; it != _end; it++)
			{
				container.insert(*it);
			}
			return std::move(container);
		}
	};

	template<typename T>
	class linq : public linq_enumerable<iterators::hide_type_iterator<T>>
	{
	public:
		linq()
		{
		}

		template<typename TIterator>
		linq(const linq_enumerable<TIterator>& e)
			:linq_enumerable<iterators::hide_type_iterator<T>>(
				iterators::hide_type_iterator<T>(e.begin()),
				iterators::hide_type_iterator<T>(e.end())
				)
		{
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