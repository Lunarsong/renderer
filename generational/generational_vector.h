#pragma once

#include <cassert>
#include <vector>
#include "generational.h"

template <typename T>
class GenerationalVector {
 public:
  template <typename... Args>
  Generational::Handle Create(Args&&... args);
  Generational::Handle Create(T&& data);
  void Destroy(Generational::Handle handle);

  T& operator[](Generational::Handle handle);
  const T& operator[](Generational::Handle handle) const;

 private:
  std::vector<T> vector_;
  Generational::Manager manager_;
};

template <typename T>
template <typename... Args>
Generational::Handle GenerationalVector<T>::Create(Args&&... args) {
  Generational::Handle handle = manager_.Create();
  auto idx = handle.Index() - 1;
  if (vector_.size() <= idx) {
    vector_.emplace_back(std::forward<Args>(args)...);
  } else {
    vector_[idx] = T(std::forward<Args>(args)...);
  }
  return handle;
}

template <typename T>
Generational::Handle GenerationalVector<T>::Create(T&& data) {
  Generational::Handle handle = manager_.Create();
  auto idx = handle.Index() - 1;
  if (vector_.size() <= idx) {
    vector_.emplace_back(data);
  } else {
    vector_[idx] = data;
  }
  return handle;
}

template <typename T>
void GenerationalVector<T>::Destroy(Generational::Handle handle) {
  assert(manager_.IsAlive(handle) && "Handle already destroyed!");
  manager_.Destroy(handle);
}

template <typename T>
T& GenerationalVector<T>::operator[](Generational::Handle handle) {
  assert(manager_.IsAlive(handle) && "Invalid handle!");
  return vector_[handle.Index() - 1];
}
template <typename T>
const T& GenerationalVector<T>::operator[](Generational::Handle handle) const {
  assert(manager_.IsAlive(handle) && "Invalid handle!");
  return vector_[handle.Index() - 1];
}
