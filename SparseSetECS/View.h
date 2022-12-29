#pragma once

#include "Registry.h"

namespace ECS {
	template <typename T>
	class SingleView {
	private:
		ComponentPool* m_Pool;

	public:
		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = T;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			pointer m_Current;

		public:
			Iterator(pointer ptr) : m_Current(ptr) {}

			reference operator*() const { return *m_Current; }
			pointer operator->() { return m_Current; }

			Iterator& operator++() {
				m_Current++;

				return *this;
			}

			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);

				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Current == b.m_Current; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Current != b.m_Current; }
		};

		Iterator begin() {
			return Iterator(m_Pool->begin<T>().GetPtr());
		}

		Iterator end() {
			return Iterator(m_Pool->end<T>().GetPtr());
		}

		SingleView(ComponentPool* pool) : m_Pool(pool) {}
	};
}
