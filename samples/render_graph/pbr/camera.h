#pragma once

#include <glm/glm.hpp>

struct Camera {
  glm::vec3 position;
  glm::mat4 view;
  glm::mat4 projection;
};
