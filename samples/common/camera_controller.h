#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <GLFW/glfw3.h>

struct CameraController {
  glm::mat4 view = glm::identity<glm::mat4>();
  glm::quat rotation = glm::identity<glm::quat>();
  glm::vec3 position = glm::vec3(0.0f);
  float pitch = 0.0f;
  float yaw = 0.0f;
};

void UpdateCameraController(CameraController& camera, GLFWwindow* window,
                            double delta_seconds);