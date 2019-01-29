#pragma once

#include <RenderAPI/RenderAPI.h>
#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 color;
  glm::vec3 normal;

  static const RenderAPI::VertexLayout layout;
};