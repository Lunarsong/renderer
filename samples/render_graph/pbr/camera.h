#pragma once

#include <glm/glm.hpp>

struct Camera {
  float near_clip;
  float far_clip;

  glm::vec3 position;
  glm::mat4 view;
  glm::mat4 projection;
};
