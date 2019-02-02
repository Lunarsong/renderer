#include "camera.h"

// Projection.
void Camera::SetPerspective(float field_of_view_degrees, float width,
                            float height, float near, float far) {
  SetPerspective(field_of_view_degrees, width / height, near, far);
}
void Camera::SetPerspective(float field_of_view_degrees, float aspect,
                            float near, float far) {
  near_clip_ = near;
  far_clip_ = far;
  aspect_ = aspect;
  field_of_view_ = field_of_view_degrees;

  mat_projection_ =
      glm::perspective(glm::radians(field_of_view_degrees), aspect, near, far);
  mat_projection_[1][1] *= -1.0f;
}

// Camera.
void Camera::LookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up) {
  orientation_ = glm::quatLookAt(at - eye, up);
  position_ = eye;
}

void Camera::SetPosition(glm::vec3 eye) { position_ = eye; }
void Camera::SetOrientation(glm::quat orientation) {
  orientation_ = orientation;
}

// Gets.
const glm::mat4& Camera::GetProjection() const { return mat_projection_; }
const glm::mat4 Camera::GetView() const {
  glm::mat4 view = glm::mat4_cast(inverse(orientation_));
  view = glm::translate(view, -position_);
  return view;
}

const glm::mat4 Camera::GetWorld() const {
  glm::mat4 model = glm::mat4_cast(orientation_);
  model = glm::translate(model, position_);
  return model;
}

const glm::vec3& Camera::GetPosition() const { return position_; }
const glm::quat& Camera::GetOrientation() const { return orientation_; }
float Camera::NearClip() const { return near_clip_; }
float Camera::FarClip() const { return far_clip_; }
float Camera::Aspect() const { return aspect_; }
float Camera::FieldOfView() const { return field_of_view_; }

Camera::Camera() {
  SetPerspective(45.0f, 4.0 / 3.0, 1920 / 1200, 0.25, 250.0);
  orientation_ = glm::identity<glm::quat>();
  position_ = glm::vec3(0.0f);
}