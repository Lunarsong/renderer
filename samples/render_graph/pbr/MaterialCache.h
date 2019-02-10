#pragma once

#include <Renderer/Material.h>
#include <string>
#include <unordered_map>
#include <utility>

class MaterialCache {
 public:
  void Cache(std::string name, uint32_t bits, Material* material);
  Material* Get(std::string name, uint32_t bits);

  ~MaterialCache();

 private:
  using KeyType = std::pair<std::string, uint32_t>;
  class HashFunction {
   public:
    size_t operator()(KeyType const& key) const {
      return std::hash<std::string>{}(key.first) + key.second;
    }
  };
  std::unordered_map<KeyType, Material*, HashFunction> materials_;
};