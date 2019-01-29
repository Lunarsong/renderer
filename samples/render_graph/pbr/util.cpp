#include "util.h"

#include <RenderAPI/RenderAPI.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>
#include "vertex.h"

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

void CreateSphere(float radius, unsigned int num_rings,
                  unsigned int num_sectors,
                  std::vector<glm::vec3>* out_vertices,
                  std::vector<unsigned int>* out_indices) {
  const float ring_step = 1.0f / static_cast<float>(num_rings - 1);
  const float sector_step = 1.0f / static_cast<float>(num_sectors - 1);

  out_vertices->reserve(num_rings * num_sectors);
  out_indices->reserve(num_rings * num_sectors * 6);
  for (unsigned int ring = 0; ring < num_rings; ++ring) {
    for (unsigned int sector = 0; sector < num_sectors; ++sector) {
      const float y = sin(-M_PI_2 + M_PI * ring * ring_step);
      const float x =
          cos(2 * M_PI * sector * sector_step) * sin(M_PI * ring * ring_step);
      const float z =
          sin(2 * M_PI * sector * sector_step) * sin(M_PI * ring * ring_step);

      out_vertices->emplace_back(x * radius, y * radius, z * radius);

      // u = sector * sector_step;
      // v = ring * ring_step;

      // Create indices.

      out_indices->emplace_back(ring * num_sectors + sector + 1);
      out_indices->emplace_back((ring + 1) * num_sectors + sector);
      out_indices->emplace_back(ring * num_sectors + sector);

      out_indices->emplace_back((ring + 1) * num_sectors + sector + 1);
      out_indices->emplace_back((ring + 1) * num_sectors + sector);
      out_indices->emplace_back(ring * num_sectors + sector + 1);
    }
  }
}

std::vector<glm::vec2> CreateSphereUVs(unsigned int num_rings,
                                       unsigned int num_sectors) {
  const float ring_step = 1.0f / static_cast<float>(num_rings - 1);
  const float sector_step = 1.0f / static_cast<float>(num_sectors - 1);
  std::vector<glm::vec2> uvs;
  uvs.reserve(num_rings * num_sectors);
  for (unsigned int ring = 0; ring < num_rings; ++ring) {
    for (unsigned int sector = 0; sector < num_sectors; ++sector) {
      uvs.emplace_back(sector * sector_step, 1.0f - ring * ring_step);
    }
  }
  return uvs;
}

Model CreateSphereModel(RenderAPI::Device device,
                        RenderAPI::CommandPool command_pool) {
  std::vector<glm::vec3> positions;
  std::vector<unsigned int> indices;
  CreateSphere(0.5, 128, 128, &positions, &indices);
  std::vector<glm::vec2> uvs = CreateSphereUVs(128, 128);
  std::vector<Vertex> vertices(positions.size());

  Model model;
  Primitive primitive;

  for (size_t i = 0; i < vertices.size(); ++i) {
    vertices[i].position = positions[i];
    vertices[i].uv = uvs[i];
    vertices[i].color = glm::vec3(1.0f);
    vertices[i].normal = glm::normalize(positions[i]);
  }

  primitive.vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kVertexBuffer,
      sizeof(Vertex) * vertices.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.vertex_buffer,
                                   vertices.data(),
                                   sizeof(Vertex) * vertices.size());

  // Create the index buffer in GPU memory and copy the data.
  primitive.index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kIndexBuffer,
      sizeof(uint32_t) * indices.size(), RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.index_buffer,
                                   indices.data(),
                                   sizeof(uint32_t) * indices.size());

  primitive.num_primitives = indices.size();

  model.primitives.emplace_back(std::move(primitive));
  return model;
}