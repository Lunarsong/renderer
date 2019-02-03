#pragma once

#include <Renderer/BuilderBase.h>

template <typename T>
BuilderBase<T>::BuilderBase() noexcept : impl_(new T) {}

template <typename T>
template <typename... args>
BuilderBase<T>::BuilderBase(args&&... args) noexcept
    : impl_(new T(std::forward<args>(args)...)) {}

template <typename T>
BuilderBase<T>::~BuilderBase() noexcept {
  delete impl_;
}

template <typename T>
BuilderBase<T>::BuilderBase(BuilderBase const& rhs) noexcept
    : impl_(new T(*rhs.impl_)) {}

template <typename T>
BuilderBase<T>& BuilderBase<T>::operator=(BuilderBase<T> const& rhs) noexcept {
  *impl_ = *rhs.impl_;
  return *this;
}
