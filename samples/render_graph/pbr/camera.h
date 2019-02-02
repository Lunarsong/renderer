#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

class Camera {
 public:
  enum class Projection {
    kPerspective,
    kOrthographic,
  };

  // Projection.
  void SetPerspective(float field_of_view_degrees, float width, float height,
                      float near, float far);
  void SetPerspective(float field_of_view_degrees, float aspect, float near,
                      float far);

  // Camera.
  void LookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up);
  void SetPosition(glm::vec3 eye);
  void SetOrientation(glm::quat orientation);

  // Gets.
  const glm::mat4& GetProjection() const;
  const glm::mat4 GetView() const;
  const glm::mat4 GetWorld() const;
  const glm::vec3& GetPosition() const;
  const glm::quat& GetOrientation() const;
  float NearClip() const;
  float FarClip() const;
  float Aspect() const;
  float FieldOfView() const;

  Camera();

 private:
  glm::vec3 position_;
  glm::quat orientation_;

  Projection projection_ = Projection::kPerspective;

  float near_clip_;
  float far_clip_;
  float field_of_view_;
  float aspect_;

  glm::mat4 mat_projection_;
};
