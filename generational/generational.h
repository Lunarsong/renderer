#pragma once

#include <cstdint>
#include <queue>
#include <vector>

namespace Generational {
using HandleType = uint64_t;
constexpr HandleType kInvalidHandle = 0;
struct Handle {
  HandleType id;

  Handle() = default;
  Handle(HandleType id);
  Handle(HandleType index, uint8_t generation);
  HandleType Index() const;
  HandleType Generation() const;
  operator HandleType() const;
};

class Manager {
 public:
  Manager();
  Handle Create();
  bool IsAlive(Handle handle) const;
  void Destroy(Handle handle);

  void Reset();

 private:
  std::queue<HandleType> free_indices_;
  std::vector<uint8_t> generations_;
};

}  // namespace Generational