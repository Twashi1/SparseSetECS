#pragma once

#include "ComponentPool.h"

namespace ECS {
	class Registry {
	private:
		std::vector<ComponentPool> m_Pools;
		ECS_SIZE_TYPE m_DefaultCapacity = 0;

	public:
		Registry(ECS_SIZE_TYPE default_capacity = 1000);

		void Resize(ECS_SIZE_TYPE new_capacity);

		void FreeEntity(const Entity& entity);

		[[nodiscard]] Entity Create();

		template <typename T>
		void RegisterComponent() {
			ECS_SIZE_TYPE id = ComponentAllocator<T>::GetID();

			if (m_Pools.size() <= id) {
				m_Pools.emplace_back(dynamic_cast<ComponentAllocatorBase*>(new ComponentAllocator<T>{}));
			}
		}

		template <typename T>
		void AddComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools.size() <= comp_id) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool& pool = m_Pools[comp_id];

			pool.Push<T>(entity, std::forward<T>(comp));
		}

		template <typename T>
		void UpdateComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools.size() <= comp_id) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool& pool = m_Pools[comp_id];

			pool.Update<T>(entity, std::forward<T>(comp));
		}

		template <typename T>
		void RemoveComponent(const Entity& entity) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools.size() <= comp_id) {
				LogWarn("Pool was not registered before use, can't remove component when pool doesn't exist");

				return;
			}

			ComponentPool& pool = m_Pools[comp_id];

			pool.FreeEntity(entity);
		}

		template <typename T>
		T* GetComponent(const Entity& entity) {
			ComponentPool& pool = m_Pools[ComponentAllocator<T>::GetID()];

			return pool.GetComponentForEntity<T>(entity);
		}

		// TODO: single entity view
	};
}
