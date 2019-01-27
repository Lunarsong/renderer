#pragma once

#include <RenderAPI/RenderAPI.h>
#include <vector>
#include "model.h"

struct IndirectLight {
  RenderAPI::ImageView reflections;
  RenderAPI::ImageView irradiance;
};

struct Skybox {
  RenderAPI::ImageView sky;
  RenderAPI::DescriptorSet descriptor = RenderAPI::kInvalidHandle;
};

struct Scene {
  std::vector<Model> models;

  IndirectLight indirect_light;
  Skybox skybox;

  RenderAPI::DescriptorSet descriptor = RenderAPI::kInvalidHandle;
};

inline void DestroyScene(RenderAPI::Device device, Scene& scene) {
  for (auto& model : scene.models) {
    DestroyModel(device, model);
  }
  scene.models.clear();
}