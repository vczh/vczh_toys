#pragma once
#include <memory>

namespace vczh
{
	struct gc_record;
	class enable_gc;
	template<typename T>
	class gc_ptr;

	struct gc_record
	{
		void*				start = nullptr;
		int					length = 0;
		enable_gc*			handle = nullptr;
	};

	class enable_gc
	{
		template<typename T>
		friend class gc_ptr;

		template<typename T, typename ...TArgs>
		friend gc_ptr<T> make_gc(TArgs&& ...args);
	private:
		gc_record			record;

		void				set_record(gc_record _record);
	public:
		enable_gc();
		virtual ~enable_gc();
	};

	namespace unsafe_functions
	{
		extern void gc_alloc(gc_record record);
		extern void gc_register(void* reference, enable_gc* handle);
		extern void gc_ref_alloc(void** handle_reference, void* handle);
		extern void gc_ref_dealloc(void** handle_reference, void* handle);
		extern void gc_ref(void** handle_reference, void* old_handle, void* new_handle);
	}
	extern void gc_start(size_t step_size, size_t max_size);
	extern void gc_stop();
	extern void gc_force_collect();

	template<typename T>
	class gc_ptr
	{
		template<typename T2>
		friend class gc_ptr;

		template<typename T2, typename ...TArgs>
		friend gc_ptr<T2> make_gc(TArgs&& ...args);

		template<typename T2, typename U>
		friend gc_ptr<T2> static_gc_cast(const gc_ptr<U>& ptr);

		template<typename T2, typename U>
		friend gc_ptr<T2> dynamic_gc_cast(const gc_ptr<U>& ptr);
	private:
		T*					reference = nullptr;

		static void* handle_of(T* reference)
		{
			return reference ? static_cast<enable_gc*>(reference)->record.start : nullptr;
		}

		gc_ptr(T* _reference)
			:reference(_reference)
		{
			unsafe_functions::gc_ref_alloc((void**)this, handle_of(reference));
		}
	public:
		gc_ptr()
		{
			unsafe_functions::gc_ref_alloc((void**)this, nullptr);
		}

		gc_ptr(const gc_ptr<T>& ptr)
			:reference(ptr.reference)
		{
			unsafe_functions::gc_ref_alloc((void**)this, handle_of(reference));
		}

		gc_ptr(gc_ptr<T>&& ptr)
			:reference(ptr.reference)
		{
			unsafe_functions::gc_ref_alloc((void**)this, handle_of(reference));
			ptr.reference = nullptr;
			unsafe_functions::gc_ref((void**)&ptr, handle_of(reference), nullptr);
		}

		template<typename U>
		gc_ptr(const gc_ptr<U>& ptr)
			:reference(ptr.reference)
		{
			unsafe_functions::gc_ref_alloc((void**)this, handle_of(reference));
		}

		~gc_ptr()
		{
			unsafe_functions::gc_ref_dealloc((void**)this, handle_of(reference));
		}

		operator bool()const
		{
			return reference != nullptr;
		}

		gc_ptr<T>& operator=(const gc_ptr<T>& ptr)
		{
			void* old_handle = handle_of(reference);
			reference = ptr.reference;
			void* new_handle = handle_of(reference);
			unsafe_functions::gc_ref((void**)this, old_handle, new_handle);
			return *this;
		}

		T* operator->()
		{
			return reference;
		}
	};

	template<typename T, typename ...TArgs>
	gc_ptr<T> make_gc(TArgs&& ...args)
	{
		void* memory = malloc(sizeof(T));
		gc_record record;
		record.start = memory;
		record.length = sizeof(T);
		unsafe_functions::gc_alloc(record);

		T* reference = new(memory)T(std::forward<TArgs>(args)...);
		enable_gc* e = static_cast<enable_gc*>(reference);
		record.handle = e;
		e->set_record(record);
		unsafe_functions::gc_register(memory, e);

		auto ptr = gc_ptr<T>(reference);
		unsafe_functions::gc_ref(nullptr, memory, nullptr);
		return ptr;
	}

	template<typename T, typename U>
	gc_ptr<T> static_gc_cast(const gc_ptr<U>& ptr)
	{
		return gc_ptr<T>(ptr);
	}

	template<typename T, typename U>
	gc_ptr<T> dynamic_gc_cast(const gc_ptr<U>& ptr)
	{
		return gc_ptr<T>(dynamic_cast<T*>(ptr.reference));
	}

#define ENABLE_GC			public virtual ::vczh::enable_gc
}
