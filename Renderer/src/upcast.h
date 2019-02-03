#pragma once

#define IMPL_UPCAST(CLASS)                                       \
  inline CLASS##Impl& upcast(CLASS& that) noexcept {             \
    return static_cast<CLASS##Impl&>(that);                      \
  }                                                              \
  inline const CLASS##Impl& upcast(const CLASS& that) noexcept { \
    return static_cast<const CLASS##Impl&>(that);                \
  }                                                              \
  inline CLASS##Impl* upcast(CLASS* that) noexcept {             \
    return static_cast<CLASS##Impl*>(that);                      \
  }                                                              \
  inline CLASS##Impl const* upcast(CLASS const* that) noexcept { \
    return static_cast<CLASS##Impl const*>(that);                \
  }