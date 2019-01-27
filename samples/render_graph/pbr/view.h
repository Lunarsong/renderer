#pragma once

#include <RenderAPI/RenderAPI.h>
#include "camera.h"

struct View {
  Camera camera;
  RenderAPI::Viewport viewport;
};