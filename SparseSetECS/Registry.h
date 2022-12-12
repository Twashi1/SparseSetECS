#pragma once

#include "ComponentPool.h"

namespace ECS {
	template <typename... Ts>
	class View;
	template <typename T>
	class SingleView;

	class Registry {
	private:
		std::array<ComponentPool*, ECS_MAX_COMPONENTS> m_Pools;
		ECS_SIZE_TYPE m_DefaultCapacity = 0;
		ECS_SIZE_TYPE m_NextEntity = 0;

	public:
		Registry(ECS_SIZE_TYPE default_capacity = 1000);

		void Resize(ECS_SIZE_TYPE new_capacity);

		void FreeEntity(const Entity& entity);

		[[nodiscard]] Entity Create();

		template <typename T>
		void RegisterComponent() {
			ECS_SIZE_TYPE id = ComponentAllocator<T>::GetID();

			if (m_Pools[id] != nullptr) {
				LogWarn("Component {} already registered, ignoring call", typeid(T).name());
			}

			// Otherwise add component pool to pools		
			m_Pools[id] = new ComponentPool(dynamic_cast<ComponentAllocatorBase*>(new ComponentAllocator<T>{}));
			m_Pools[id]->Resize(m_DefaultCapacity);
		}

		template <typename T>
		void AddComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			pool->Push<T>(entity, std::forward<T>(comp));
		}

		template <typename T>
		void UpdateComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			pool->Update<T>(entity, std::forward<T>(comp));
		}

		template <typename T>
		void RemoveComponent(const Entity& entity) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, can't remove component when pool doesn't exist");

				return;
			}

			ComponentPool*& pool = m_Pools[comp_id];

			pool->FreeEntity(entity);
		}

		template <typename T>
		T* GetComponent(const Entity& entity) {
			// TODO: assert pool not nullptr
			ComponentPool*& pool = m_Pools[ComponentAllocator<T>::GetID()];

			return pool->GetComponentForEntity<T>(entity);
		}

		template <typename T>
		bool HasComponent(const Entity& entity) {
			return m_Pools[ComponentAllocator<T>::GetID()]->Contains(entity);
		}

		template <typename... Ts>
		bool AnyOf(const Entity& entity) {
			return (HasComponent<Ts>(entity) || ...);
		}

		template <typename... Ts>
		bool AllOf(const Entity& entity) {
			return (HasComponent<Ts>(entity) && ...);
		}

		template <typename... Ts>
		std::tuple<Ts*...> GetComponents(const Entity& entity) {
			return std::make_tuple<Ts*...>(GetComponent<Ts>(entity)...);
		}

		template <typename... Ts>
		friend class View;
		template <typename T>
		friend class SingleView;

		template <typename... Ts>
		View<Ts...> CreateView() {
			// Check all pools are allocated
			if (((m_Pools[ComponentAllocator<Ts>::GetID()] == nullptr) || ...)) {
				LogFatal("Can't create view: one of the components is not registered!");
			}

			return View<Ts...>(this);
		}

		template <typename T>
		SingleView<T> CreateSingleView() {
			if (m_Pools[ComponentAllocator<T>::GetID()] == nullptr) {
				LogFatal("Can't create view, object type {} is not registered!", typeid(T).name());
			}

			return SingleView<T>(m_Pools[ComponentAllocator<T>::GetID()]);
		}
	};
}
