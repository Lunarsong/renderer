#include "MaterialCache.h"

#include <cassert>

void MaterialCache::Cache(std::string name, uint32_t bits, Material* material) {
  auto pair = std::make_pair(std::move(name), bits);
  assert(materials_.count(pair) == 0);
  materials_.insert(std::make_pair(pair, material));
}

Material* MaterialCache::Get(std::string name, uint32_t bits) {
  auto pair = std::make_pair(std::move(name), bits);
  auto it = materials_.find(pair);
  if (it == materials_.end()) {
    return nullptr;
  }
  return it->second;
}

MaterialCache::~MaterialCache() {
  for (auto& it : materials_) {
    Material::Destroy(it.second);
  }
}