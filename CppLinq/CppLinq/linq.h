#pragma once

#include <utility>
#include <xutility>
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

	template<typename T, typename U>
	struct zip_pair
	{
		T				first;
		U				second;

		zip_pair(){}
		zip_pair(const T& _first, const U& _second) :first(_first), second(_second){}
		template<typename X, typename Y>
		zip_pair(const zip_pair<X, Y>& p) :first(p.first), second(p.second){}

		static int compare(const zip_pair<T, U>& a, const zip_pair<T, U>& b)
		{
			return
				a.first<a.second ? -1 :
				a.first>a.second ? 1 :
				b.first<b.second ? -1 :
				b.first>b.second ? 1 :
				0;
		}

		bool operator==(const zip_pair<T, U>& p)const{ return first == p.first && second == p.second; }
		bool operator!=(const zip_pair<T, U>& p)const{ return first != p.first || second != p.second; }
		bool operator<(const zip_pair<T, U>& p)const{ return compare(*this, p) < 0; }
		bool operator<=(const zip_pair<T, U>& p)const{ return compare(*this, p) <= 0; }
		bool operator>(const zip_pair<T, U>& p)const{ return compare(*this, p) > 0; }
		bool operator>=(const zip_pair<T, U>& p)const{ return compare(*this, p) >= 0; }
	};

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
				typedef iterator_interface								TSelf;
			public:
				virtual std::shared_ptr<TSelf>	next() = 0;
				virtual T						deref() = 0;
				virtual bool					equals(const std::shared_ptr<TSelf>& it) = 0;
			};

			template<typename TIterator>
			class iterator_implement : public iterator_interface
			{
				typedef iterator_implement<TIterator>					TSelf;
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

			typedef hide_type_iterator<T>								TSelf;

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
		// storage
		//////////////////////////////////////////////////////////////////

		template<typename T>
		class storage_iterator
		{
			typedef storage_iterator<T>									TSelf;
		private:
			std::shared_ptr<std::vector<T>>		values;
			typename std::vector<T>::iterator	iterator;

		public:
			storage_iterator(const std::shared_ptr<std::vector<T>>& _values, const typename std::vector<T>::iterator& _iterator)
				:values(_values), iterator(_iterator)
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

			T operator*()const
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
		// select :: [T] -> (T -> U) -> [U]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator, typename TFunction>
		class select_iterator
		{
			typedef select_iterator<TIterator, TFunction>				TSelf;
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
			typedef where_iterator<TIterator, TFunction>				TSelf;
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
		// skip :: [T] -> int -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator>
		class skip_iterator
		{
			typedef skip_iterator<TIterator>							TSelf;
		private:
			TIterator			iterator;
			TIterator			end;

		public:
			skip_iterator(const TIterator& _iterator, const TIterator& _end, int _count)
				:iterator(_iterator), end(_end)
			{
				for (int i = 0; i < _count && iterator != end; i++, iterator++);
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
		// skip_while :: [T] -> (T -> bool) -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator, typename TFunction>
		class skip_while_iterator
		{
			typedef skip_while_iterator<TIterator, TFunction>			TSelf;
		private:
			TIterator			iterator;
			TIterator			end;
			TFunction			f;

		public:
			skip_while_iterator(const TIterator& _iterator, const TIterator& _end, const TFunction& _f)
				:iterator(_iterator), end(_end), f(_f)
			{
				while (iterator != end && f(*iterator))
				{
					iterator++;
				}
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
		// take :: [T] -> int -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator>
		class take_iterator
		{
			typedef take_iterator<TIterator>							TSelf;
		private:
			TIterator			iterator;
			TIterator			end;
			int					count;
			int					current;

		public:
			take_iterator(const TIterator& _iterator, const TIterator& _end, int _count)
				:iterator(_iterator), end(_end), count(_count), current(0)
			{
				if (current == count)
				{
					iterator = end;
				}
			}

			TSelf& operator++()
			{
				if (++current == count)
				{
					iterator = end;
				}
				else
				{
					iterator++;
				}
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				if (++current == count)
				{
					iterator = end;
				}
				else
				{
					iterator++;
				}
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
		// take_while :: [T] -> (T -> bool) -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator, typename TFunction>
		class take_while_iterator
		{
			typedef take_while_iterator<TIterator, TFunction>			TSelf;
		private:
			TIterator			iterator;
			TIterator			end;
			TFunction			f;

		public:
			take_while_iterator(const TIterator& _iterator, const TIterator& _end, const TFunction& _f)
				:iterator(_iterator), end(_end), f(_f)
			{
				if (iterator != end && !f(*iterator))
				{
					iterator = end;
				}
			}

			TSelf& operator++()
			{
				if (!f(*++iterator))
				{
					iterator = end;
				}
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				if (!f(*++iterator))
				{
					iterator = end;
				}
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
		// concat :: [T] -> [T] -> [T]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator1, typename TIterator2>
		class concat_iterator
		{
			typedef concat_iterator<TIterator1, TIterator2>				TSelf;
		private:
			TIterator1			current1;
			TIterator1			end1;
			TIterator2			current2;
			TIterator2			end2;
			bool				first;

		public:
			concat_iterator(const TIterator1& _current1, const TIterator1& _end1, const TIterator2& _current2, const TIterator2& _end2)
				:current1(_current1), end1(_end1), current2(_current2), end2(_end2), first(_current1 != _end1)
			{
			}

			TSelf& operator++()
			{
				if (first)
				{
					if (++current1 == end1)
					{
						first = false;
					}
				}
				else
				{
					current2++;
				}
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				if (first)
				{
					if (++current1 == end1)
					{
						first = false;
					}
				}
				else
				{
					current2++;
				}
				return t;
			}

			iterator_type<TIterator1> operator*()const
			{
				return first ? *current1 : *current2;
			}

			bool operator==(const TSelf& it)const
			{
				if (first != it.first) return false;
				return first ? current1 == it.current1 : current2 == it.current2;
			}

			bool operator!=(const TSelf& it)const
			{
				if (first != it.first) return true;
				return first ? current1 != it.current1 : current2 != it.current2;
			}
		};

		//////////////////////////////////////////////////////////////////
		// zip :: [T] -> [U] -> [(T, U)]
		//////////////////////////////////////////////////////////////////

		template<typename TIterator1, typename TIterator2>
		class zip_iterator
		{
			typedef zip_iterator<TIterator1, TIterator2>							TSelf;
			typedef zip_pair<iterator_type<TIterator1>, iterator_type<TIterator2>>	TElement;
		private:
			TIterator1			current1;
			TIterator1			end1;
			TIterator2			current2;
			TIterator2			end2;

		public:
			zip_iterator(const TIterator1& _current1, const TIterator1& _end1, const TIterator2& _current2, const TIterator2& _end2)
				:current1(_current1), end1(_end1), current2(_current2), end2(_end2)
			{
			}

			TSelf& operator++()
			{
				if (current1 != end1 && current2 != end2)
				{
					current1++;
					current2++;
				}
				return *this;
			}

			TSelf operator++(int)
			{
				TSelf t = *this;
				if (current1 != end1 && current2 != end2)
				{
					current1++;
					current2++;
				}
				return t;
			}

			TElement operator*()const
			{
				return TElement(*current1, *current2);
			}

			bool operator==(const TSelf& it)const
			{
				return current1 == it.current1 && current2 == it.current2;
			}

			bool operator!=(const TSelf& it)const
			{
				return current1 != it.current1 || current2 != it.current2;
			}
		};
	}

	template<typename TIterator>
	class linq_enumerable;

	namespace types
	{
		template<typename T>
		using storage_it = iterators::storage_iterator<T>;

		template<typename TIterator, typename TFunction>
		using select_it = iterators::select_iterator<TIterator, TFunction>;

		template<typename TIterator, typename TFunction>
		using where_it = iterators::where_iterator<TIterator, TFunction>;

		template<typename TIterator>
		using skip_it = iterators::skip_iterator<TIterator>;

		template<typename TIterator, typename TFunction>
		using skip_while_it = iterators::skip_while_iterator<TIterator, TFunction>;

		template<typename TIterator>
		using take_it = iterators::take_iterator<TIterator>;

		template<typename TIterator, typename TFunction>
		using take_while_it = iterators::take_while_iterator<TIterator, TFunction>;

		template<typename TIterator1, typename TIterator2>
		using concat_it = iterators::concat_iterator<TIterator1, TIterator2>;

		template<typename TIterator1, typename TIterator2>
		using zip_it = iterators::zip_iterator<TIterator1, TIterator2>;
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

	template<typename TIterator>
	class linq_enumerable;

	template<typename T>
	class linq;

	template<typename TElement>
	linq<TElement> from_values(std::shared_ptr<std::vector<TElement>> xs)
	{
		return linq_enumerable<types::storage_it<TElement>>(
			types::storage_it<TElement>(xs, xs->begin()),
			types::storage_it<TElement>(xs, xs->end())
			);
	}

	template<typename TElement>
	linq<TElement> from_values(const std::initializer_list<TElement>& ys)
	{
		auto xs = std::make_shared<std::vector<TElement>>(ys);
		return linq_enumerable<types::storage_it<TElement>>(
			types::storage_it<TElement>(xs, xs->begin()),
			types::storage_it<TElement>(xs, xs->end())
			);
	}

	template<typename TElement>
	linq<TElement> from_value(const TElement& value)
	{
		auto xs = std::make_shared<std::vector<TElement>>();
		xs->push_back(value);
		return from_values(xs);
	}

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

#define SUPPORT_STL_CONTAINERS(NAME)\
	template<typename TContainer>\
	auto NAME(const TContainer& e)const->decltype(NAME(from(e)))\
		{\
		return NAME(from(e)); \
		}\
		template<typename TElement>\
		auto NAME(const std::initializer_list<TElement>& e)const->decltype(NAME(from(e)))\
		{\
		return NAME(from(e)); \
		}\

		//////////////////////////////////////////////////////////////////
		// iterating (lazy evaluation)
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

		linq_enumerable<types::skip_it<TIterator>> skip(int count)const
		{
			return linq_enumerable<types::skip_it<TIterator>>(
				types::skip_it<TIterator>(_begin, _end, count),
				types::skip_it<TIterator>(_end, _end, count)
				);
		}

		template<typename TFunction>
		linq_enumerable<types::skip_while_it<TIterator, TFunction>> skip_while(const TFunction& f)const
		{
			return linq_enumerable<types::skip_while_it<TIterator, TFunction>>(
				types::skip_while_it<TIterator, TFunction>(_begin, _end, f),
				types::skip_while_it<TIterator, TFunction>(_end, _end, f)
				);
		}

		linq_enumerable<types::take_it<TIterator>> take(int count)const
		{
			return linq_enumerable<types::take_it<TIterator>>(
				types::take_it<TIterator>(_begin, _end, count),
				types::take_it<TIterator>(_end, _end, count)
				);
		}

		template<typename TFunction>
		linq_enumerable<types::take_while_it<TIterator, TFunction>> take_while(const TFunction& f)const
		{
			return linq_enumerable<types::take_while_it<TIterator, TFunction>>(
				types::take_while_it<TIterator, TFunction>(_begin, _end, f),
				types::take_while_it<TIterator, TFunction>(_end, _end, f)
				);
		}

		template<typename TIterator2>
		linq_enumerable<types::concat_it<TIterator, TIterator2>> concat(const linq_enumerable<TIterator2>& e)const
		{
			return linq_enumerable<types::concat_it<TIterator, TIterator2>>(
				types::concat_it<TIterator, TIterator2>(_begin, _end, e.begin(), e.end()),
				types::concat_it<TIterator, TIterator2>(_end, _end, e.end(), e.end())
				);
		}
		SUPPORT_STL_CONTAINERS(concat)

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
				return from_value(value);
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
			if (it != _end) throw linq_exception("The collection should have exactly one value.");

			return *this;
		}

		linq<TElement> single_or_default(const TElement& value)const
		{
			auto it = _begin;
			if (it == _end) return from_value(value);

			it++;
			if (it != _end) throw linq_exception("The collection should have exactly one value.");

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
		SUPPORT_STL_CONTAINERS(sequence_equal)

			//////////////////////////////////////////////////////////////////
			// set
			//////////////////////////////////////////////////////////////////

			linq<TElement> distinct()const
		{
				std::set<TElement> set;
				auto xs = std::make_shared<std::vector<TElement>>();
				for (auto it = _begin; it != _end; it++)
				{
					if (set.insert(*it).second)
					{
						xs->push_back(*it);
					}
				}
				return from_values(xs);
			}

		template<typename TIterator2>
		linq<TElement> except_with(const linq_enumerable<TIterator2>& e)const
		{
			std::set<TElement> set(e.begin(), e.end());
			auto xs = std::make_shared<std::vector<TElement>>();
			for (auto it = _begin; it != _end; it++)
			{
				if (set.insert(*it).second)
				{
					xs->push_back(*it);
				}
			}
			return from_values(xs);
		}
		SUPPORT_STL_CONTAINERS(except_with)

			template<typename TIterator2>
		linq<TElement> intersect_with(const linq_enumerable<TIterator2>& e)const
		{
			std::set<TElement> seti, set(e.begin(), e.end());
			auto xs = std::make_shared<std::vector<TElement>>();
			for (auto it = _begin; it != _end; it++)
			{
				if (seti.insert(*it).second && !set.insert(*it).second)
				{
					xs->push_back(*it);
				}
			}
			return from_values(xs);
		}
		SUPPORT_STL_CONTAINERS(intersect_with)

			template<typename TIterator2>
		linq<TElement> union_with(const linq_enumerable<TIterator2>& e)const
		{
			return concat(e).distinct();
		}
		SUPPORT_STL_CONTAINERS(union_with)

			//////////////////////////////////////////////////////////////////
			// aggregating
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
		void then_by()const;

		template<typename TIterator2>
		linq_enumerable<types::zip_it<TIterator, TIterator2>> zip_with(const linq_enumerable<TIterator2>& e)const
		{
			return linq_enumerable<types::zip_it<TIterator, TIterator2>>(
				types::zip_it<TIterator, TIterator2>(_begin, _end, e.begin(), e.end()),
				types::zip_it<TIterator, TIterator2>(_end, _end, e.end(), e.end())
				);
		}
		SUPPORT_STL_CONTAINERS(zip_with)

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

#undef SUPPORT_STL_CONTAINERS
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