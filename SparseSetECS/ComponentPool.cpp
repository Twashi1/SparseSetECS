#include "ComponentPool.h"
#include "Registry.h"
#include "View.h"
#include "Group.h"

namespace ECS {
	void ComponentPool::m_AllocatePackedSpace(const ECS_SIZE_TYPE& packed_index) {
		// Not enough space!
		if (m_PackedArray.capacity <= packed_index) {
			if (m_PackedArray.capacity * m_PackedArray.growth_factor <= packed_index) {
				Resize(packed_index + 1);
			}
			else {
				Resize(m_PackedArray.capacity * m_PackedArray.growth_factor);
			}
		}
	}
	
	void ComponentPool::Swap(const Entity& a, const Entity& b) {
		ECS_SIZE_TYPE& index_a = m_SparseArray[a];
		ECS_SIZE_TYPE& index_b = m_SparseArray[b];

		std::byte* location_a = &m_ComponentArray[index_a * m_Allocator->SizeInBytes()];
		std::byte* location_b = &m_ComponentArray[index_b * m_Allocator->SizeInBytes()];

		// Swap components
		m_Allocator->Swap(location_a, location_b);
		// Swap entities in packed array
		std::swap(m_PackedArray[index_a], m_PackedArray[index_b]);
		// Swap sparse set indices
		std::swap(index_a, index_b);
	}

	ECS_SIZE_TYPE ComponentPool::GetID() const { return m_ID; }

	void ComponentPool::FreeEntity(const Entity& entity) {
		ECS_SIZE_TYPE& index = m_SparseArray[entity];

		Swap(index, m_PackedArray.size - 1);
		index = dead_entity;
		--m_PackedArray.size;
		--m_ComponentArray.size;
	}

	void ComponentPool::Resize(ECS_SIZE_TYPE new_capacity)
	{
		if (new_capacity <= m_PackedArray.capacity) return;

		// Resize packed array
		{
			// Create new array
			Entity* new_data = new Entity[new_capacity];
			// Move data
			std::memcpy(new_data, m_PackedArray.data, m_PackedArray.capacity * sizeof(Entity));
			// Rest should be nulls
			std::fill_n(new_data + m_PackedArray.capacity, new_capacity - m_PackedArray.capacity, dead_entity);

			// Update capacity
			m_PackedArray.capacity = new_capacity;
			// Delete old data
			delete[] m_PackedArray.data;
			// Replace old data with ptr to new data
			m_PackedArray.data = new_data;
		}

		// Resize component array
		{
			// Create new array
			std::byte* new_data = new std::byte[new_capacity * m_Allocator->SizeInBytes()];
			// Copy data from old array to new one
			m_Allocator->AssignRange(new_data, m_ComponentArray.data, m_ComponentArray.size);

			// Update capacity
			m_ComponentArray.capacity = new_capacity;
			// Delete old data
			delete[] m_ComponentArray.data;
			// Replace old data with ptr to new data
			m_ComponentArray.data = new_data;
		}
	}

	bool ComponentPool::Contains(const Entity& entity)
	{
		return m_SparseArray[GetIdentifier(entity)] != dead_entity;
	}

	ECS_SIZE_TYPE ComponentPool::GetSize() const {
		return m_PackedArray.size;
	}
	
	ComponentPool::~ComponentPool() {
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
	
	ComponentPool::ComponentPool(ComponentPool&& other) noexcept
		: m_SparseArray(std::move(other.m_SparseArray)),
		m_PackedArray(std::move(other.m_PackedArray)),
		m_ComponentArray(std::move(other.m_ComponentArray)),
		m_Allocator(std::move(other.m_Allocator)),
		m_ID(std::move(other.m_ID))
	{}

	ComponentPool& ComponentPool::operator=(ComponentPool&& other) noexcept {
		m_SparseArray = std::move(other.m_SparseArray);
		m_PackedArray = std::move(other.m_PackedArray);
		m_ComponentArray = std::move(other.m_ComponentArray);
		m_ID = std::move(other.m_ID);
	}
	
	ComponentPool::ComponentPool(ComponentAllocatorBase* allocator)
		: m_Allocator(allocator), m_ID(allocator->GetComponentID())
	{}
}
