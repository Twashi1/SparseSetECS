#pragma once

#include "Core.h"
#include "PagedArray.h"
#include "Entity.h"
#include "Family.h"

namespace ECS {
	template <typename... Ts>
	class View;
	template <typename T>
	class SingleView;
	template <typename... Ts>
	class FullOwningGroup;

	// Base for the component allocator, defines some methods for moving data of type T
	class ComponentAllocatorBase {
	public:
		virtual ~ComponentAllocatorBase() = default;

		virtual void Assign(std::byte* dest, std::byte* src) const = 0;
		virtual void Delete(std::byte* data) const = 0;
		virtual void AssignRange(std::byte* dest, std::byte* src, ECS_SIZE_TYPE count) const = 0;
		virtual void DeleteRange(std::byte* data, ECS_SIZE_TYPE count) const = 0;
		virtual void Swap(std::byte* a, std::byte* b) const = 0;

		virtual std::size_t SizeInBytes() const = 0;
		virtual ECS_SIZE_TYPE GetComponentID() const = 0;
	};

	// For moving, deleting, and allocating data of some type T (somewhat) safely
	template <typename T>
	class ComponentAllocator final : public ComponentAllocatorBase {
	private:
		static const ECS_SIZE_TYPE m_ID;

	public:
		static constexpr ECS_SIZE_TYPE GetID() { return m_ID; }

		void Assign(std::byte* dest, std::byte* src) const override final {
			// Attempt a simple memcpy if possible
			if constexpr (std::is_trivially_constructible_v<T>) {
				memcpy(dest, src, sizeof(T));
			}
			else if constexpr (std::is_move_constructible_v<T>) {
				new (dest) T(std::move(*reinterpret_cast<T*>(src)));
			}
			else if constexpr (std::is_copy_constructible_v<T>) {
				new (dest) T(*reinterpret_cast<T*>(src));
			}
			else {
				LogFatal("Attempted to move object of {}, but no available constructor", typeid(T).name());
			}
		}

		void Delete(std::byte* data) const override final {
			T* ptr = std::launder(reinterpret_cast<T*>(data));

			// Call deconstructor
			ptr->~T();
		}
		
		void AssignRange(std::byte* dest, std::byte* src, ECS_SIZE_TYPE count) const override final {
			// Attempt simple memcpy of entire range if possible (very fast)
			if constexpr (std::is_trivially_constructible_v<T>) {
				memcpy(dest, src, count * sizeof(T));
			}
			// Otherwise we must do member-wise move/copy (very slow)
			else if constexpr (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>) {
				// Iterate each member
				for (ECS_SIZE_TYPE i = 0; i < count; i++) {
					// Assign each member
					Assign(dest + i * sizeof(T), src + i * sizeof(T));
				}
			}
			else {
				LogFatal("Attempted to move objects of {}, but no available constructor", typeid(T).name());
			}
		}

		void DeleteRange(std::byte* data, ECS_SIZE_TYPE count) const override final {
			// If not trivially destructible, we must cll delete for each member
			if constexpr (!std::is_trivially_destructible_v<T>) {
				// Iterate each member
				for (ECS_SIZE_TYPE i = 0; i < count; i++) {
					// Delete member
					Delete(data + i * sizeof(T));
				}
			}
			// Otherwise do nothing
		}

		void Swap(std::byte* a, std::byte* b) const override final {
			// Allocate space to store a temporary
			std::byte* tmp = new std::byte[sizeof(T)];

			// Some move shenanigans
			Assign(tmp, a);
			Assign(a,   b);
			Assign(b, tmp);
		}

		std::size_t SizeInBytes() const override final {
			return sizeof(T);
		}

		ECS_SIZE_TYPE GetComponentID() const override final {
			return ComponentAllocator<T>::GetID();
		}

		ComponentAllocator() = default;
		~ComponentAllocator() {}

		ComponentAllocator(ComponentAllocator&& other) noexcept = default;
		ComponentAllocator(const ComponentAllocator& other) = default;

		ComponentAllocator& operator=(const ComponentAllocator& other) = default;
		ComponentAllocator& operator=(ComponentAllocator&& other) noexcept = default;
	};

	template <typename T>
	const ECS_SIZE_TYPE ComponentAllocator<T>::m_ID = Family::Type<T>();

	template <typename T, float _growth_factor = 2.0f>
	struct PackedArray {
		static constexpr float growth_factor = _growth_factor;

		T* data;
		ECS_SIZE_TYPE capacity;
		ECS_SIZE_TYPE size;

		T& operator[](const ECS_SIZE_TYPE& index) {
			return data[index];
		}

		PackedArray()
			: capacity(0), size(0), data(nullptr)
		{}

		PackedArray(PackedArray&& other) noexcept
			: data(std::move(other.data)), capacity(std::move(other.capacity)), size(std::move(other.size))
		{
			other.data = nullptr;
			other.capacity = 0;
			other.size = 0;
		}

		PackedArray(const PackedArray& other) = delete;
		PackedArray& operator=(const PackedArray& other) = delete;

		PackedArray& operator=(PackedArray&& other) noexcept
		{
			data = std::move(other.data);
			capacity = std::move(other.capacity);
			size = std::move(other.size);

			other.data = nullptr;
			other.capacity = 0;
			other.size = 0;
		}
	};

	class Registry;

	struct ComponentPool {
	private:
		PagedArray<Entity, ECS_SPARSE_PAGE, ECS_ENTITY_MAX, dead_entity> m_SparseArray;

		PackedArray<Entity>		m_PackedArray;
		PackedArray<std::byte>	m_ComponentArray;

		ComponentAllocatorBase*	m_Allocator = nullptr;

		void m_AllocatePackedSpace(const ECS_SIZE_TYPE& packed_index);

		template <typename T>
		T* m_Index(const ECS_SIZE_TYPE& index) {
			return reinterpret_cast<T*>(&m_ComponentArray[index * m_Allocator->SizeInBytes()]);
		}

		ECS_SIZE_TYPE m_ID = 0;

	public:
		template <typename T>
		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = T;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			pointer m_Ptr;

		public:
			Iterator(pointer ptr) : m_Ptr(ptr) {}

			pointer& GetPtr() { return m_Ptr; }
			
			reference operator*() const { return *m_Ptr; }
			pointer operator->() { return m_Ptr; }

			Iterator& operator++() { m_Ptr++; return *this; }
			Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Ptr == b.m_Ptr; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Ptr != b.m_Ptr; }
		};
		
		template <typename T>
		Iterator<T> begin() { return Iterator<T>(&m_ComponentArray.data[0]); }
		template <typename T>
		Iterator<T> end()	{ return Iterator<T>(&m_ComponentArray.data[m_ComponentArray.size * m_Allocator->SizeInBytes()]); }

		template <typename T>
		T* GetComponentForEntity(const Entity& entity) {
			ECS_SIZE_TYPE packed_index = m_SparseArray[GetIdentifier(entity)];

			if (packed_index == dead_entity) {
				LogError("Attempted to index entity {} in pool type {}, but entity doesn't exist in this pool", entity, typeid(T).name());
				// Indicates that this entity doesn't exist, no need to go to packed array
				return nullptr;
			}

			return reinterpret_cast<T*>(&m_ComponentArray[packed_index * m_Allocator->SizeInBytes()]);
		}

		template <typename T>
		void Push(const Entity& entity, T&& comp) {
			if (m_SparseArray[GetIdentifier(entity)] != dead_entity) {
				LogError("Entity {} already had component {}; can't push!", entity, typeid(T).name());

				return;
			}

			// Update index to be at end of packed list
			ECS_SIZE_TYPE packed_index = m_PackedArray.size;
			m_SparseArray[GetIdentifier(entity)] = packed_index;

			// Ensure enough space for this index
			m_AllocatePackedSpace(packed_index);

			// Add entity into packed array
			m_PackedArray[packed_index] = entity;

			// Add component into component array
			std::byte* location = &m_ComponentArray[packed_index * m_Allocator->SizeInBytes()];

			m_Allocator->Assign(location, reinterpret_cast<std::byte*>(&comp));

			// Increment size of both arrays
			++m_PackedArray.size;
			++m_ComponentArray.size;
		}

		template <typename T>
		void Replace(const Entity& entity, T&& comp) {
			// Get index of entity in sparse array
			ECS_SIZE_TYPE& packed_index = m_SparseArray[GetIdentifier(entity)];

			// If entity doesn't exist
			if (packed_index == dead_entity) {
				LogWarn("Entity {} doesn't have component {}, calling Push<{}> for you...", entity, typeid(T).name(), typeid(T).name());

				Push<T>(entity, std::forward<T>(comp));
				
				return;
			}

			// Get location of component
			std::byte* location = &m_ComponentArray[packed_index * m_Allocator->SizeInBytes()];

			// Update component
			m_Allocator->Assign(location, reinterpret_cast<std::byte*>(&comp));
		}

		void Swap(const Entity& a, const Entity& b);

		// TODO: fix when version control is implemented
		void FreeEntity(const Entity& entity);

		void Resize(ECS_SIZE_TYPE new_capacity);

		bool Contains(const Entity& entity);

		ECS_SIZE_TYPE GetSize() const;

		~ComponentPool() {
			if (m_PackedArray.data != nullptr) {
				delete[] m_PackedArray.data;
			}

			if (m_ComponentArray.size > 0) {
				m_Allocator->DeleteRange(&m_ComponentArray[0], m_ComponentArray.capacity);

				delete[] m_ComponentArray.data;
			}

			if (m_Allocator != nullptr) {
				delete m_Allocator;
			}
		}

		ComponentPool(ComponentPool&& other) noexcept
			: m_SparseArray(std::move(other.m_SparseArray)),
			m_PackedArray(std::move(other.m_PackedArray)),
			m_ComponentArray(std::move(other.m_ComponentArray)),
			m_Allocator(std::move(other.m_Allocator)),
			m_ID(std::move(other.m_ID))
		{}

		ComponentPool(const ComponentPool& other) = delete;

		ComponentPool& operator=(ComponentPool&& other) noexcept {
			m_SparseArray = std::move(other.m_SparseArray);
			m_PackedArray = std::move(other.m_PackedArray);
			m_ComponentArray = std::move(other.m_ComponentArray);
			m_ID = std::move(other.m_ID);
		}

		ComponentPool& operator=(const ComponentPool& other) = delete;

		ComponentPool(ComponentAllocatorBase* allocator)
			: m_Allocator(allocator), m_ID(allocator->GetComponentID())
		{}

		friend class Registry;

		template <typename... Ts>
		friend class View;
		template <typename T>
		friend class SingleView;
		template <typename... Ts>
		friend class FullOwningGroup;
	};
}
