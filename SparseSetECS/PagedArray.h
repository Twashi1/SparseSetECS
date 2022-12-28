#pragma once

#include "Core.h"

namespace ECS {
	template <typename T, ECS_SIZE_TYPE _page_size, ECS_SIZE_TYPE _capacity>
	struct PagedArray {
	private:
		static const ECS_SIZE_TYPE m_PageSize = _page_size;
		static const ECS_SIZE_TYPE m_Pages	  = ((_capacity - 1) / m_PageSize + 1);
		static const ECS_SIZE_TYPE m_Capacity = m_Pages * m_PageSize;

	public:
		using value_type = T;
		using page_type = value_type*;
		using book_type = std::array<page_type, m_Pages>;
		
	private:
		T m_Default = T{};
		book_type m_Book;		// A collection of pages is a book?

		inline page_type& m_AllocateOrGetPage(const ECS_SIZE_TYPE& page_index) {
			page_type& page = m_Book[page_index];

			// If page not allocated
			if (page == nullptr) {
				// Allocate
				page = new T[m_PageSize];

				// Fill with default
				// TODO: verify this is actually setting the correct amount of memory
				std::fill_n(page, m_PageSize, m_Default);

				// Return
				return page;
			}
			// Otherwise just return page
			else {
				return page;
			}
		}

		inline page_type& m_GetPage(const ECS_SIZE_TYPE& page_index) {
			return m_Book[page_index];
		}

		inline T& m_Index(const ECS_SIZE_TYPE& index) {
			// Calculate the page that index is stored in
			ECS_SIZE_TYPE page_index = index / m_PageSize;
			// Calculate index within the page for the given index
			ECS_SIZE_TYPE index_in_page = index - (page_index * m_PageSize);

			// Get the relevant page
			page_type& page = m_AllocateOrGetPage(page_index);

			// Return element at that index in the page
			return page[index_in_page];
		}

		inline const T& m_Index(const ECS_SIZE_TYPE& index) const {
			// Calculate the page that index is stored in
			ECS_SIZE_TYPE page_index = index / m_PageSize;
			// Calculate index within the page for the given index
			ECS_SIZE_TYPE index_in_page = index - (page_index * m_PageSize);

			// Get the relevant page
			page_type& page = m_GetPage(page_index);

			if (page != nullptr) {
				return page[index_in_page];
			}
			else {
				return m_Default;
			}
		}

	public:
		static const ECS_SIZE_TYPE GetCapacity()  { return m_Capacity; }
		static const ECS_SIZE_TYPE GetPageCount() { return m_Pages; }

		void SetDefault(const T& new_default) { m_Default = new_default; }

		PagedArray() { std::fill(m_Book.begin(), m_Book.end(), nullptr); }
		PagedArray(const PagedArray& other) = delete;
		PagedArray(PagedArray&& other) noexcept
			: m_Book(std::move(other.m_Book)), m_Default(std::move(other.m_Default))
		{
			std::fill(other.m_Book.begin(), other.m_Book.end(), nullptr);
		}

		PagedArray& operator=(const PagedArray& other) = delete;
		PagedArray& operator=(PagedArray&& other) noexcept {
			m_Book = std::move(other.m_Book);
			m_Default = std::move(other.m_Default);

			// TODO: memcpy might be slightly faster?
			// Set everything in other array to nulls
			std::fill(other.m_Book.begin(), other.m_Book.end(), nullptr);

			return *this;
		}

		T& operator[](const ECS_SIZE_TYPE& index)				{ return m_Index(index); }
		const T& operator[](const ECS_SIZE_TYPE& index) const	{ return m_Index(index); }
	};
}
