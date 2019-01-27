#include "camera_controller.h"

/*struct CameraController {
  glm::mat4 view = glm::identity<glm::mat4>;
  glm::quat rotation = glm::identity<glm::quat>;
  glm::vec3 position = glm::vec3(0.0f);
};*/

void UpdateCameraController(CameraController& camera, GLFWwindow* window,
                            double delta_seconds) {
  static double x = 0;
  static double y = 0;
  static const float kSpeed = 4.0f;

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  double dx = xpos - x;
  double dy = ypos - y;
  x = xpos;
  y = ypos;
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    camera.pitch += dy * 1 / 60.0f;
    camera.yaw += dx * 1 / 60.0f;
  }

  glm::quat pitch = glm::angleAxis(camera.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::quat yaw = glm::angleAxis(camera.yaw, glm::vec3(0.0f, 1.0f, 0.0f));

  glm::vec3 move(0.0f);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    move.z += 1.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    move.z -= 1.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    move.x += 1.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    move.x -= 1.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    move.y += 1.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
    move.y -= 1.0f;
  }

  camera.position += yaw * move * kSpeed * static_cast<float>(delta_seconds);
  camera.view = glm::mat4_cast(inverse(camera.rotation));
  camera.view = glm::translate(camera.view, -camera.position);
  camera.rotation = yaw * pitch;
}