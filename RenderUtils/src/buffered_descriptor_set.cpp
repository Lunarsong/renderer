#include "../include/RenderUtils/buffered_descriptor_set.h"

BufferedDescriptorSet BufferedDescriptorSet::Create(
    RenderAPI::Device device, RenderAPI::DescriptorSetPool pool,
    RenderAPI::DescriptorSetLayout layout) {
  std::vector<RenderAPI::DescriptorSetLayout> layouts(3, layout);
  BufferedDescriptorSet set;
  RenderAPI::AllocateDescriptorSets(pool, layouts, set.sets_);
  return set;
}

RenderAPI::DescriptorSet BufferedDescriptorSet::operator++() {
  current_ = (current_ + 1) % BufferedDescriptorSet::kBufferCount;
  return sets_[current_];
}

BufferedDescriptorSet::operator RenderAPI::DescriptorSet&() {
  return sets_[current_];
}

BufferedDescriptorSet::operator const RenderAPI::DescriptorSet&() const {
  return sets_[current_];
}

BufferedDescriptorSet::operator RenderAPI::DescriptorSet*() {
  return &sets_[current_];
}

BufferedDescriptorSet::operator const RenderAPI::DescriptorSet*() const {
  return &sets_[current_];
}
