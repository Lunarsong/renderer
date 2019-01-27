#include "util.h"

#include <RenderAPI/RenderAPI.h>

Model CreateCubeModel(RenderAPI::Device device,
                      RenderAPI::CommandPool command_pool) {
  Model model;

  Primitive primitive;
  // Create the cube vertices.
  // Position, UV, Color, Normal.
  std::vector<float> cube = {
      // Font face.
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, -1.0f,  //
      0.5, 0.5, -0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, -1.0f,    //
      -0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, -1.0f,   //
      0.5, -0.5, -0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, -1.0f,

      // Back face.
      0.5, -0.5, 0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f,  //
      -0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f,  //
      0.5, 0.5, 0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f,   //
      -0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 0.0f, 1.0f,

      // Left face.
      -0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, -1.0f, 0.0f, 0.0f,   //
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, -1.0f, 0.0f, 0.0f,  //
      -0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, -1.0f, 0.0f, 0.0f,   //
      -0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0f, 0.0f, 0.0f,

      // Right face.
      0.5, 0.5, -0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f,   //
      0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f,  //
      0.5, -0.5, 0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f,   //
      0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0f, 0.0f, 0.0f,

      // Bottom face.
      -0.5, -0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0f, -1.0f, 0.0f,  //
      0.5, -0.5, -0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0f, -1.0f, 0.0f,   //
      -0.5, -0.5, 0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0f, -1.0f, 0.0f,   //
      0.5, -0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0f, -1.0f, 0.0f,

      // Top face.
      -0.5, 0.5, -0.5f, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f,  //
      0.5, 0.5, -0.5f, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f,   //
      -0.5, 0.5, 0.5f, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f,   //
      0.5, 0.5, 0.5f, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f     //
  };

  primitive.vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kVertexBuffer,
      sizeof(float) * cube.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.vertex_buffer,
                                   cube.data(), sizeof(float) * cube.size());

  // Create the index buffer in GPU memory and copy the data.
  const std::vector<uint32_t> indices = {
      0,  1,  2,  3,  1,  0,   // Front face
      4,  5,  6,  7,  5,  4,   // Back face
      10, 9,  8,  8,  11, 10,  // Left face
      12, 13, 14, 14, 15, 12,  // Right face
      19, 17, 16, 16, 18, 19,  // Bottom face
      20, 21, 23, 23, 22, 20,  // Top face

  };
  primitive.index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kIndexBuffer,
      sizeof(uint32_t) * indices.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.index_buffer,
                                   indices.data(),
                                   sizeof(uint32_t) * indices.size());

  primitive.num_primitives = 6 * 6;

  model.primitives.emplace_back(std::move(primitive));
  return model;
}