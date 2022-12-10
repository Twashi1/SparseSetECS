#pragma once

#include "Core.h"
#include "PagedArray.h"
#include "Entity.h"
#include "Family.h"

namespace ECS {
	class ComponentAllocatorBase {
	public:
		virtual ~ComponentAllocatorBase() = default;

		virtual void Assign(std::byte* dest, std::byte* src) const = 0;
		virtual void Delete(std::byte* data) const = 0;
		virtual void AssignRange(std::byte* dest, std::byte* src, ECS_SIZE_TYPE count) const = 0;
		virtual void DeleteRange(std::byte* data, ECS_SIZE_TYPE count) const = 0;
		virtual void Swap(std::byte* a, std::byte* b) const = 0;

		virtual std::size_t SizeInBytes() const = 0;
	};

	template <typename T>
	class ComponentAllocator final : public ComponentAllocatorBase {
	private:
		static const ECS_SIZE_TYPE m_ID;

	public:
		static constexpr ECS_SIZE_TYPE GetID() { return m_ID; }

		void Assign(std::byte* dest, std::byte* src) const override final {
			if constexpr (std::is_trivially_constructible_v<T>) {
				// We can do a simple memcpy
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
			if constexpr (std::is_trivially_constructible_v<T>) {
				// We can do a simple memcpy
				memcpy(dest, src, count * sizeof(T));
			}
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
			// If not trivially destructible, call delete for each member
			if constexpr (!std::is_trivially_destructible_v<T>) {
				// Iterate each member
				for (ECS_SIZE_TYPE i = 0; i < count; i++) {
					// Delete member
					Delet(data + i * sizeof(T));
				}
			}
		}

		void Swap(std::byte* a, std::byte* b) const override final {
			// Allocate space to store a temporary
			std::byte* tmp = new std::byte[sizeof(T)];

			Assign(tmp, a);
			Assign(a,   b);
			Assign(b, tmp);
		}

		std::size_t SizeInBytes() const override final {
			return sizeof(T);
		}

		ComponentAllocator() = default;

		ComponentAllocator(ComponentAllocator&& other) noexcept = default;
		ComponentAllocator(const ComponentAllocator& other) = default;

		ComponentAllocator& operator=(const ComponentAllocator& other) = default;
		ComponentAllocator& operator=(ComponentAllocator&& other) noexcept = default;
	};

	template <typename T>
	const ECS_SIZE_TYPE ComponentAllocator<T>::m_ID = Family<ComponentsFamily>::Type<T>();

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

		ComponentAllocatorBase*	m_Allocator;

		void m_AllocatePackedSpace(const ECS_SIZE_TYPE& packed_index);

	public:
		template <typename T>
		T* GetComponentForEntity(const Entity& entity) {
			ECS_SIZE_TYPE packed_index = m_SparseArray[entity];

			if (GetIdentifier(packed_index) == null_entity) {
				LogError("Attempted to index entity {} in pool type {}, but entity doesn't exist in this pool", entity, typeid(T).name());
				// Indicates that this entity doesn't exist, no need to go to packed array
				return nullptr;
			}

			return reinterpret_cast<T*>(&m_ComponentArray[packed_index * m_Allocator->SizeInBytes()]);
		}

		template <typename T>
		void Push(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE packed_index = m_SparseArray[entity];

			if (GetIdentifier(packed_index) != null_entity) {
				LogError("Entity {} already had component {}; can't push!", entity, typeid(T).name());

				return;
			}

			// Update index to be at end of packed list
			packed_index = m_PackedArray.size;

			// Update sparse array index
			ECS_SIZE_TYPE& index = m_SparseArray[entity];
			index = m_PackedArray.size << ECS_ENTITY_SHIFT_ALIGN;

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
		void Update(const Entity& entity, T&& comp) {
			// Get index of entity in sparse array
			ECS_SIZE_TYPE& packed_index = m_SparseArray[entity];

			// If entity doesn't exist
			if (GetIdentifier(packed_index) == null_entity) {
				LogWarn("Entity {} doesn't have component {}, calling Push<{}> for you...", entity, typeid(T).name(), typeid(T).name());

				Push<T>(entity, std::forward<T>(comp));
				
				return;
			}

			// Get location of component
			std::byte* location = &m_ComponentArray[packed_index * m_Allocator->SizeInBytes()];

			m_Allocator->Assign(location, reinterpret_cast<std::byte*>(&comp));
		}

		void Swap(const Entity& a, const Entity& b);

		// TODO: fix when version control
		void FreeEntity(const Entity& entity);

		void Resize(ECS_SIZE_TYPE new_capacity);

		bool Contains(const Entity& entity);

		~ComponentPool() {
			delete[] m_PackedArray.data;

			m_Allocator->DeleteRange(&m_ComponentArray[0], m_ComponentArray.capacity);

			delete m_Allocator;
		}

		ComponentPool(ComponentPool&& other) noexcept
			: m_SparseArray(std::move(other.m_SparseArray)),
			m_PackedArray(std::move(other.m_PackedArray)),
			m_ComponentArray(std::move(other.m_ComponentArray)),
			m_Allocator(std::move(other.m_Allocator))
		{}

		ComponentPool(const ComponentPool& other) = delete;

		ComponentPool& operator=(ComponentPool&& other) noexcept {
			m_SparseArray = std::move(other.m_SparseArray);
			m_PackedArray = std::move(other.m_PackedArray);
			m_ComponentArray = std::move(other.m_ComponentArray);
		}

		ComponentPool& operator=(const ComponentPool& other) = delete;

		ComponentPool(ComponentAllocatorBase* allocator)
			: m_Allocator(allocator)
		{}

		friend class Registry;
	};
}
