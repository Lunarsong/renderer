#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/MaterialInstance.h>
#include <glm/glm.hpp>
#include <vector>

struct Texture {
  RenderAPI::Image image = RenderAPI::kInvalidHandle;
  RenderAPI::ImageView texture = RenderAPI::kInvalidHandle;
  RenderAPI::Sampler sampler = RenderAPI::kInvalidHandle;
};

struct MetallicRoughnessMaterial {
  enum AlphaMode { kOpaque, kMask, kBlend };

  glm::vec4 base_color_factor = glm::vec4(1.0f);
  glm::vec4 emissive_factor = glm::vec4(0.0f);
  float metallic_factor = 1.0f;
  float roughness_factor = 1.0f;

  Texture base_color_texture;
  Texture metallic_roughness_texture;
  Texture normal_texture;
  Texture occlusion_texture;
  Texture emissive_texture;

  AlphaMode alpha_mode = kOpaque;
  float alpha_cutoff = 0.5f;
  bool double_sided = false;

  RenderAPI::DescriptorSet descriptor_set;
};

struct MetallicRoughnessMaterialGpuData {
  glm::vec4 uBaseColor = glm::vec4(1.0f);
  glm::vec2 uMetallicRoughness = glm::vec2(0.0f);
  float uAmbientOcclusion = 1.0f;
};

struct Primitive {
  RenderAPI::Buffer vertex_buffer = RenderAPI::kInvalidHandle;
  RenderAPI::Buffer index_buffer = RenderAPI::kInvalidHandle;
  uint32_t num_primitives = 0;
  uint32_t first_index = 0;
  uint32_t vertex_offset = 0;

  MaterialInstance* material = nullptr;
};

struct Mesh {
  std::vector<Primitive> primitives;
  glm::mat4 mat_world = glm::mat4(1.0f);
};

inline void DestroyTexture(RenderAPI::Device device, Texture& texture) {
  if (texture.sampler != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroySampler(device, texture.sampler);
    texture.sampler = RenderAPI::kInvalidHandle;
  }
  if (texture.texture != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyImageView(device, texture.texture);
    texture.texture = RenderAPI::kInvalidHandle;
  }
  if (texture.image != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyImage(texture.image);
    texture.image = RenderAPI::kInvalidHandle;
  }
}

inline void DestroyMesh(RenderAPI::Device device, Mesh& mesh) {
  for (auto& primitive : mesh.primitives) {
    if (primitive.vertex_buffer != RenderAPI::kInvalidHandle) {
      RenderAPI::DestroyBuffer(primitive.vertex_buffer);
      primitive.vertex_buffer = RenderAPI::kInvalidHandle;
    }
    if (primitive.index_buffer != RenderAPI::kInvalidHandle) {
      RenderAPI::DestroyBuffer(primitive.index_buffer);
      primitive.index_buffer = RenderAPI::kInvalidHandle;
    }

    MaterialInstance::Destroy(primitive.material);
  }
}