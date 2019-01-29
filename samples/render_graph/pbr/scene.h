#pragma once

#include <RenderAPI/RenderAPI.h>
#include <vector>
#include "model.h"

struct IndirectLight {
  RenderAPI::ImageView reflections;
  RenderAPI::ImageView irradiance;
  RenderAPI::ImageView brdf;
};

struct Skybox {
  RenderAPI::ImageView sky;
};

struct Scene {
  std::vector<Model> models;

  IndirectLight indirect_light;
  Skybox skybox;

  inline void SetSkybox(RenderAPI::ImageView sky) { skybox.sky = sky; }
  inline void SetIndirectLight(RenderAPI::ImageView irradiance,
                               RenderAPI::ImageView reflections,
                               RenderAPI::ImageView brdf) {
    indirect_light.irradiance = irradiance;
    indirect_light.reflections = reflections;
    indirect_light.brdf = brdf;
  }
};

inline void DestroyScene(RenderAPI::Device device, Scene& scene) {
  for (auto& model : scene.models) {
    DestroyModel(device, model);
  }
  scene.models.clear();
}