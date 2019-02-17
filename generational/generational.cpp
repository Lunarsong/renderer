#include "generational.h"

#include <cassert>

namespace Generational {
namespace {
constexpr HandleType kGenerationBits = 8;
constexpr HandleType kGenerationMask = (1 << kGenerationBits) - 1;
constexpr HandleType kIndexBits = (sizeof(HandleType) * 8) - kGenerationBits;
constexpr HandleType kIndexMask = (HandleType(1) << kIndexBits) - 1;
constexpr HandleType kMinimumFreeIndices = 1024;
}  // namespace

Handle::Handle(HandleType index, uint8_t generation) {
  id = (static_cast<HandleType>(generation) << kIndexBits) | index;
}
Handle::Handle(HandleType id) : id(id) {}
HandleType Handle::Index() const { return id & kIndexMask; }
HandleType Handle::Generation() const {
  return (id >> kIndexBits) & kGenerationMask;
}

Handle::operator HandleType() const { return id; }

Handle Manager::Create() {
  HandleType idx;
  if (free_indices_.size() > kMinimumFreeIndices) {
    idx = free_indices_.front();
    free_indices_.pop();
  } else {
    generations_.push_back(0);
    idx = generations_.size() - 1;
    assert(idx < ((HandleType)1 << kIndexBits));
  }
  return Handle(idx, generations_[idx]);
}

bool Manager::IsAlive(Handle handle) const {
  return generations_[handle.Index()] ==
         static_cast<uint8_t>(handle.Generation());
}

void Manager::Destroy(Handle handle) {
  const unsigned idx = handle.Index();
  ++generations_[idx];
  free_indices_.push(idx);
}

void Manager::Reset() {
  generations_.resize(1);
  while (!free_indices_.empty()) {
    free_indices_.pop();
  }
}

Manager::Manager() { generations_.resize(1); }

}  // namespace Generational