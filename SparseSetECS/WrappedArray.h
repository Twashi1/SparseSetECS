#pragma once

#include "Core.h"

template <typename T>
struct WrappedArray {
	T* data = nullptr;
	ECS_SIZE_TYPE capacity = 0;
	ECS_SIZE_TYPE size = 0;

	T& operator[](const ECS_SIZE_TYPE& index) { return data[index]; }
	const T& operator[](const ECS_SIZE_TYPE& index) const { return data[index]; }

	WrappedArray() = default;

	WrappedArray(WrappedArray&& other) noexcept
		: data(std::move(other.data)), capacity(std::move(other.capacity)), size(std::move(other.size))
	{
		other.data = nullptr;
		other.size = 0;
		other.capacity = 0;
	}

	WrappedArray(const WrappedArray&) = delete;

	WrappedArray& operator=(WrappedArray&& other) noexcept
	{
		data = std::move(other.data);
		capacity = std::move(other.capacity);
		size = std::move(other.size);

		other.data = nullptr;
		other.capacity = 0;
		other.size = 0;

		return *this;
	}

	WrappedArray& operator=(const WrappedArray&) = delete;
};
