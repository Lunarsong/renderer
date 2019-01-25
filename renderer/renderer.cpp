#include "include/renderer/renderer.h"

namespace RenderAPI {
bool IsDepthStencilFormat(TextureFormat format) {
  return (format >= TextureFormat::kD16_UNORM &&
          format <= TextureFormat::kD32_SFLOAT_S8_UINT);
}
};  // namespace RenderAPI