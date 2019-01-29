#include "vertex.h"

const RenderAPI::VertexLayout Vertex::layout = {
    {RenderAPI::VertexAttributeType::kVec3, 0},
    {RenderAPI::VertexAttributeType::kVec2, sizeof(float) * 3},
    {RenderAPI::VertexAttributeType::kVec3, sizeof(float) * 5},
    {RenderAPI::VertexAttributeType::kVec3, sizeof(float) * 8}};