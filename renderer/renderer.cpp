#include "include/renderer/renderer.h"

namespace Renderer {
bool IsDepthStencilFormat(TextureFormat format) {
  return (format >= TextureFormat::kD16_UNORM &&
          format <= TextureFormat::kD32_SFLOAT_S8_UINT);
}
};  // namespace Renderer