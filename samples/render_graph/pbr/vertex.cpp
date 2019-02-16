#include "vertex.h"

const RenderAPI::VertexInputState Vertex::layout = {
    {{0, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT, 0},
     {1, 0, RenderAPI::TextureFormat::kR32G32_SFLOAT, sizeof(float) * 3},
     {2, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT, sizeof(float) * 5},
     {3, 0, RenderAPI::TextureFormat::kR32G32B32_SFLOAT, sizeof(float) * 8}},
    {{0, sizeof(float) * 11, RenderAPI::VertexInputRate::kVertex}}};
