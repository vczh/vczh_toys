#include "gc_ptr.h"
#include <assert.h>
#include <algorithm>
#include <set>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;

namespace vczh
{
	//////////////////////////////////////////////////////////////////
	// enable_gc
	//////////////////////////////////////////////////////////////////

	void enable_gc::set_record(gc_record _record)
	{
		record = _record;
	}

	enable_gc::enable_gc()
	{
	}

	enable_gc::~enable_gc()
	{
	}

	//////////////////////////////////////////////////////////////////
	// helper functions
	//////////////////////////////////////////////////////////////////

	struct gc_handle
	{
		static const int				counter_range = (int32_t)0x80000000;

		int								counter = 0;
		gc_record						record;
		multiset<gc_handle*>			references;
		multiset<void**>				handle_references;
		bool							mark = false;
	};

	struct gc_handle_dummy
	{
		int								counter = 0;
		gc_record						record;
	};

	struct gc_handle_comparer
	{
		bool operator ()(gc_handle* a, gc_handle* b)
		{
			if (a->counter == gc_handle::counter_range)
			{
				return a->record.start < b->record.start;
			}
			if (b->counter == gc_handle::counter_range)
			{
				return (intptr_t)(a->record.start) + a->record.length <= (intptr_t)b->record.start;
			}
			else
			{
				return a->record.start < b->record.start;
			}
		}
	};

	typedef set<gc_handle*, gc_handle_comparer>		gc_handle_container;
	mutex								gc_lock;
	gc_handle_container*				gc_handles = nullptr;
	size_t								gc_step_size = 0;
	size_t								gc_max_size = 0;
	size_t								gc_last_current_size = 0;
	size_t								gc_current_size = 0;

	gc_handle* gc_find_unsafe(void* handle)
	{
		gc_handle_dummy dummy;
		dummy.record.start = handle;
		gc_handle* input = reinterpret_cast<gc_handle*>(&dummy);
		auto it = gc_handles->find(input);
		return it == gc_handles->end() ? nullptr : *it;
	}

	gc_handle* gc_find_parent_unsafe(void** handle_reference)
	{
		gc_handle_dummy dummy;
		dummy.counter = gc_handle::counter_range;
		dummy.record.start = (void*)handle_reference;
		gc_handle* input = reinterpret_cast<gc_handle*>(&dummy);
		auto it = gc_handles->find(input);
		return it == gc_handles->end() ? nullptr : *it;
	}

	void gc_ref_connect_unsafe(void** handle_reference, void* handle, bool alloc)
	{
		gc_handle* parent = nullptr;
		if (alloc)
		{
			if (parent = gc_find_parent_unsafe(handle_reference))
			{
				parent->handle_references.insert(handle_reference);
			}
		}
		if (auto target = gc_find_unsafe(handle))
		{
			if (parent || (parent = gc_find_parent_unsafe(handle_reference)))
			{
				parent->references.insert(target);
				if (alloc)
				{
					parent->handle_references.insert(handle_reference);
				}
			}
			else
			{
				target->counter++;
			}
		}
	}

	void gc_ref_disconnect_unsafe(void** handle_reference, void* handle, bool dealloc)
	{
		gc_handle* parent = nullptr;
		if (dealloc)
		{
			if (parent = gc_find_parent_unsafe(handle_reference))
			{
				parent->handle_references.insert(handle_reference);
			}
		}
		if (auto target = gc_find_unsafe(handle))
		{
			if (parent || (parent = gc_find_parent_unsafe(handle_reference)))
			{
				if (dealloc)
				{
					parent->handle_references.erase(handle_reference);
				}
				parent->references.erase(target);
			}
			else
			{
				target->counter--;
			}
		}
	}

	void gc_destroy_disconnect_unsafe(gc_handle* handle)
	{
		for (auto handle_reference : handle->handle_references)
		{
			auto x = reinterpret_cast<gc_ptr<enable_gc>*>(handle_reference);
			*handle_reference = nullptr;
		}
	}

	void gc_destroy_unsafe(gc_handle* handle)
	{
		handle->record.handle->~enable_gc();
		gc_current_size -= handle->record.length;
		free(handle->record.start);
		delete handle;
	}

	void gc_destroy_unsafe(vector<gc_handle*>& garbages)
	{
		for (auto handle : garbages)
		{
			gc_destroy_disconnect_unsafe(handle);
		}
		for (auto handle : garbages)
		{
			gc_destroy_unsafe(handle);
		}
		gc_last_current_size = gc_current_size;
	}

	void gc_force_collect_unsafe(vector<gc_handle*>& garbages)
	{
		vector<gc_handle*> markings;

		for (auto handle : *gc_handles)
		{
			if (handle->mark = handle->counter > 0)
			{
				markings.push_back(handle);
			}
		}

		for (int i = 0; i < (int)markings.size(); i++)
		{
			auto ref = markings[i];
			for (auto child : ref->references)
			{
				if (!child->mark)
				{
					child->mark = true;
					markings.push_back(child);
				}
			}
		}

		for (auto it = gc_handles->begin(); it != gc_handles->end();)
		{
			if (!(*it)->mark)
			{
				auto it2 = it++;
				garbages.push_back(*it2);
				gc_handles->erase(it2);
			}
			else
			{
				it++;
			}
		}
	}

	namespace unsafe_functions
	{
		void gc_alloc(gc_record record)
		{
			assert(gc_handles);
			auto handle = new gc_handle;
			handle->record = record;
			handle->counter = 1;

			vector<gc_handle*> garbages;
			{
				lock_guard<mutex> guard(gc_lock);
				gc_handles->insert(handle);
				gc_current_size += handle->record.length;

				if (gc_current_size > gc_max_size)
				{
					gc_force_collect_unsafe(garbages);
				}
				else if (gc_current_size - gc_last_current_size > gc_step_size)
				{
					gc_force_collect_unsafe(garbages);
				}
			}
			gc_destroy_unsafe(garbages);
		}

		void gc_register(void* reference, enable_gc* handle)
		{
			assert(gc_handles);

			lock_guard<mutex> guard(gc_lock);
			gc_find_unsafe(reference)->record.handle = handle;
		}

		void gc_ref_alloc(void** handle_reference, void* handle)
		{
			assert(gc_handles);

			lock_guard<mutex> guard(gc_lock);
			gc_ref_connect_unsafe(handle_reference, handle, true);
		}

		void gc_ref_dealloc(void** handle_reference, void* handle)
		{
			assert(gc_handles);

			lock_guard<mutex> guard(gc_lock);
			gc_ref_disconnect_unsafe(handle_reference, handle, true);
		}

		void gc_ref(void** handle_reference, void* old_handle, void* new_handle)
		{
			assert(gc_handles);

			lock_guard<mutex> guard(gc_lock);
			gc_ref_disconnect_unsafe(handle_reference, old_handle, false);
			gc_ref_connect_unsafe(handle_reference, new_handle, false);
		}
	}

	void gc_start(size_t step_size, size_t max_size)
	{
		assert(!gc_handles);

		lock_guard<mutex> guard(gc_lock);
		gc_handles = new gc_handle_container;
		gc_step_size = step_size;
		gc_max_size = max_size;
		gc_last_current_size = 0;
		gc_current_size = 0;
	}

	void gc_stop()
	{
		assert(gc_handles);
		gc_force_collect();

		lock_guard<mutex> guard(gc_lock);
		auto garbages = gc_handles;
		gc_handles = nullptr;
		gc_step_size = 0;
		gc_max_size = 0;
		gc_last_current_size = 0;
		gc_current_size = 0;

		for (auto handle : *garbages)
		{
			gc_destroy_disconnect_unsafe(handle);
		}
		for (auto handle : *garbages)
		{
			gc_destroy_unsafe(handle);
		}
		delete garbages;
	}

	void gc_force_collect()
	{
		assert(gc_handles);
		
		vector<gc_handle*> garbages;
		{
			lock_guard<mutex> guard(gc_lock);
			gc_force_collect_unsafe(garbages);
		}
		gc_destroy_unsafe(garbages);
	}
}