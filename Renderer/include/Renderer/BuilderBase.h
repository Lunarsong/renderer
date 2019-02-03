#pragma once

template <typename T>
class BuilderBase {
 public:
  template <typename... args>
  explicit BuilderBase(args&&...) noexcept;
  BuilderBase() noexcept;
  ~BuilderBase() noexcept;
  BuilderBase(BuilderBase const& rhs) noexcept;
  BuilderBase& operator=(BuilderBase const& rhs) noexcept;

  BuilderBase(BuilderBase&& rhs) noexcept : impl_(rhs.impl_) {
    rhs.impl_ = nullptr;
  }
  BuilderBase& operator=(BuilderBase&& rhs) noexcept {
    auto temp = impl_;
    impl_ = rhs.impl_;
    rhs.impl_ = temp;
    return *this;
  }

 protected:
  T* impl_;
  inline T* operator->() noexcept { return impl_; }
  inline T const* operator->() const noexcept { return impl_; }
};
