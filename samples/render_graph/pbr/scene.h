#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/MaterialParams.h>
#include <glm/glm.hpp>
#include <vector>
#include "model.h"

struct DirectionalLight {
  glm::vec3 direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
};

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

  DirectionalLight directional_light;
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