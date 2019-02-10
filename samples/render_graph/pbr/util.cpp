#include "util.h"

#include <RenderAPI/RenderAPI.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "MaterialBits.h"
#include "vertex.h"

Mesh CreateCubeMesh(RenderAPI::Device device,
                    RenderAPI::CommandPool command_pool) {
  Mesh mesh;

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

  mesh.primitives.emplace_back(std::move(primitive));
  return mesh;
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

Mesh CreateSphereMesh(RenderAPI::Device device,
                      RenderAPI::CommandPool command_pool) {
  std::vector<glm::vec3> positions;
  std::vector<unsigned int> indices;
  CreateSphere(0.5, 128, 128, &positions, &indices);
  std::vector<glm::vec2> uvs = CreateSphereUVs(128, 128);
  std::vector<Vertex> vertices(positions.size());

  Mesh mesh;
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

  mesh.primitives.emplace_back(std::move(primitive));
  return mesh;
}

Mesh CreatePlaneMesh(RenderAPI::Device device,
                     RenderAPI::CommandPool command_pool) {
  Mesh mesh;
  Primitive primitive;
  Vertex vertices[] = {{
                           glm::vec3(-20.0f, -2.0f, -20.0f),
                           glm::vec2(0.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f),
                       },
                       {
                           glm::vec3(20.0f, -2.0f, -20.0f),
                           glm::vec2(1.0f, 0.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f),
                       },
                       {
                           glm::vec3(-20.0f, -2.0f, 20.0f),
                           glm::vec2(0.0f, 1.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f),
                       },
                       {
                           glm::vec3(20.0f, -2.0f, 20.0f),
                           glm::vec2(1.0f, 1.0f),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f),
                       }};

  primitive.vertex_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kVertexBuffer, sizeof(Vertex) * 4,
      RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.vertex_buffer,
                                   vertices, sizeof(Vertex) * 4);

  // Create the index buffer in GPU memory and copy the data.
  uint32_t indices[] = {0, 1, 3, 3, 2, 0};
  primitive.index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kIndexBuffer,
      sizeof(uint32_t) * 6, RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, primitive.index_buffer,
                                   indices, sizeof(uint32_t) * 6);

  primitive.num_primitives = 6;

  mesh.primitives.emplace_back(std::move(primitive));
  return mesh;
}

const float* GetGltfPrimitiveData(const char* name, const tinygltf::Model& gltf,
                                  const tinygltf::Primitive& primitive,
                                  size_t* count = nullptr,
                                  size_t* stride = nullptr) {
  // Obtain the accessor.
  const auto& attr = primitive.attributes.find(name);
  if (attr == primitive.attributes.cend()) {
    return nullptr;
  }
  const tinygltf::Accessor& accessor = gltf.accessors[attr->second];

  // Assign the count and stride.
  if (count) {
    *count = accessor.count;
  }
  if (stride) {
    *stride = (accessor.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;
  }

  // Obtains the buffer.
  const tinygltf::BufferView& buffer_view =
      gltf.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = gltf.buffers[buffer_view.buffer];

  // Retrieve the data.
  return reinterpret_cast<const float*>(
      &buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
}

std::vector<Vertex> VerticesFromGltf(const tinygltf::Model& gltf,
                                     const tinygltf::Primitive& primitive) {
  std::vector<Vertex> vertices;

  // Obtain the positions.
  size_t count;
  const float* positions =
      GetGltfPrimitiveData("POSITION", gltf, primitive, &count);
  assert(positions);

  // Allocate memory for the vertices.
  vertices.resize(count);

  const float* uvs = GetGltfPrimitiveData("TEXCOORD_0", gltf, primitive);
  const float* normals = GetGltfPrimitiveData("NORMAL", gltf, primitive);
  size_t colors_stride;
  const float* colors =
      GetGltfPrimitiveData("COLOR_0", gltf, primitive, nullptr, &colors_stride);

  // Copy the data.
  for (size_t i = 0; i < count; ++i) {
    vertices[i].position = glm::make_vec3(&positions[i * 3]);
    vertices[i].uv = uvs ? glm::make_vec2(&uvs[i * 2]) : glm::vec2(0.0f);
    vertices[i].normal =
        normals ? glm::make_vec3(&normals[i * 3]) : glm::vec3(0.0f, 1.0f, 0.0f);
    vertices[i].color =
        colors ? glm::make_vec3(&colors[i * colors_stride]) : glm::vec3(1.0f);
  }

  return vertices;
}

RenderAPI::Buffer IndexBufferFromGltf(RenderAPI::Device device,
                                      RenderAPI::CommandPool command_pool,
                                      const tinygltf::Model& gltf,
                                      const tinygltf::Primitive& primitive,
                                      uint32_t& num_primitives) {
  // Obtain the indices.
  const tinygltf::Accessor& accessor = gltf.accessors[primitive.indices];
  const tinygltf::BufferView& bufferView =
      gltf.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = gltf.buffers[bufferView.buffer];
  const void* data =
      &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

  num_primitives = accessor.count;
  uint32_t* indices = nullptr;
  const uint32_t* copy_indices = nullptr;

  // Create the index data.
  switch (accessor.componentType) {
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
      copy_indices = static_cast<const uint32_t*>(data);
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
      const uint16_t* buf = static_cast<const uint16_t*>(data);
      indices = new uint32_t[num_primitives];
      copy_indices = indices;
      for (size_t index = 0; index < accessor.count; index++) {
        indices[index] = static_cast<uint32_t>(buf[index]);
      }
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
      const uint8_t* buf = static_cast<const uint8_t*>(data);
      indices = new uint32_t[num_primitives];
      copy_indices = indices;
      for (size_t index = 0; index < accessor.count; index++) {
        indices[index] = static_cast<uint32_t>(buf[index]);
      }
      break;
    }
    default:
      std::cerr << "Index component type " << accessor.componentType
                << " not supported!" << std::endl;
      assert(false);
  }

  // Create the buffer.
  RenderAPI::Buffer index_buffer = RenderAPI::CreateBuffer(
      device, RenderAPI::BufferUsageFlagBits::kIndexBuffer,
      sizeof(uint32_t) * num_primitives, RenderAPI::MemoryUsage::kGpu);
  RenderAPI::StageCopyDataToBuffer(command_pool, index_buffer, copy_indices,
                                   sizeof(uint32_t) * num_primitives);

  if (indices) {
    delete[] indices;
  }
  return index_buffer;
}

RenderAPI::ImageView ImageAndImageViewFromGltf(
    RenderAPI::Device device, RenderAPI::CommandPool command_pool,
    const tinygltf::Image& gltf_image,
    std::vector<RenderAPI::Image>& image_cache) {
  unsigned char* buffer = nullptr;
  const unsigned char* copy_buffer = nullptr;
  size_t buffer_size = 0;
  if (gltf_image.component == 3) {
    // Most devices don't support RGB only on Vulkan so convert if necessary
    buffer_size = gltf_image.width * gltf_image.height * 4;
    buffer = new unsigned char[buffer_size];
    copy_buffer = buffer;
    unsigned char* rgba = buffer;
    const unsigned char* rgb = &gltf_image.image[0];
    for (int32_t i = 0; i < gltf_image.width * gltf_image.height; ++i) {
      for (int32_t j = 0; j < 3; ++j) {
        rgba[j] = rgb[j];
      }
      rgba += 4;
      rgb += 3;
    }
  } else {
    copy_buffer = &gltf_image.image[0];
    buffer_size = gltf_image.image.size();
  }
  RenderAPI::TextureFormat format = RenderAPI::TextureFormat::kR8G8B8A8_UNORM;

  RenderAPI::Image image = RenderAPI::CreateImage(
      device, {RenderAPI::TextureType::Texture2D, format,
               RenderAPI::Extent3D(gltf_image.width, gltf_image.height, 1)});
  RenderAPI::BufferImageCopy image_copy(0, 0, 0, gltf_image.width,
                                        gltf_image.height, 1);
  RenderAPI::StageCopyDataToImage(command_pool, image, copy_buffer, buffer_size,
                                  1, &image_copy);
  RenderAPI::ImageViewCreateInfo image_view_info(
      image, RenderAPI::ImageViewType::Texture2D, format,
      RenderAPI::ImageSubresourceRange(
          RenderAPI::ImageAspectFlagBits::kColorBit));
  RenderAPI::ImageView image_view =
      RenderAPI::CreateImageView(device, image_view_info);

  if (buffer) {
    delete[] buffer;
  }

  image_cache.emplace_back(image);
  return image_view;
}

bool MeshFromGLTF(RenderAPI::Device device, RenderAPI::CommandPool command_pool,
                  MaterialCache* material_cache, Mesh& mesh,
                  std::vector<RenderAPI::Image>& image_cache,
                  std::vector<RenderAPI::ImageView>& image_views_cache) {
  tinygltf::Model gltf;
  const char* filename = "samples/render_graph/pbr/data/DamagedHelmet.glb";
  std::string err;
  std::string warn;

  tinygltf::TinyGLTF loader;
  bool res = loader.LoadBinaryFromFile(&gltf, &err, &warn, filename);
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "ERR: " << err << std::endl;
  }

  if (!res) {
    std::cout << "Failed to load glTF: " << filename << std::endl;
    return false;
  } else {
    std::cout << "Loaded glTF: " << filename << std::endl;
  }

  std::vector<RenderAPI::ImageView> images;
  images.reserve(gltf.images.size());
  for (const auto& it : gltf.images) {
    images.push_back(
        ImageAndImageViewFromGltf(device, command_pool, it, image_cache));
  }
  image_views_cache.insert(image_views_cache.end(), images.begin(),
                           images.end());

  std::vector<MaterialInstance*> materials;
  materials.reserve(gltf.materials.size());
  for (const auto& mat : gltf.materials) {
    uint32_t bits = 0;
    RenderAPI::ImageView base_color_texture = 0;
    RenderAPI::ImageView normal_texture = 0;
    RenderAPI::ImageView occlusion_texture = 0;
    RenderAPI::ImageView metallic_roughness_texture = 0;
    RenderAPI::ImageView emissive_texture = 0;
    MetallicRoughnessMaterialGpuData data;
    if (const auto& it = mat.values.find("baseColorFactor");
        it != mat.values.cend()) {
      data.uBaseColor = glm::make_vec4(it->second.ColorFactor().data());
    }
    if (const auto& it = mat.values.find("roughnessFactor");
        it != mat.values.cend()) {
      data.uMetallicRoughness.y = static_cast<float>(it->second.Factor());
    }
    if (const auto& it = mat.values.find("metallicFactor");
        it != mat.values.cend()) {
      data.uMetallicRoughness.x = static_cast<float>(it->second.Factor());
    }

    if (const auto& it = mat.values.find("baseColorTexture");
        it != mat.values.cend()) {
      bits |= MetallicRoughnessBits::kHasBaseColorTexture;
      base_color_texture = images[it->second.TextureIndex()];
    }
    if (const auto& it = mat.additionalValues.find("normalTexture");
        it != mat.additionalValues.cend()) {
      bits |= MetallicRoughnessBits::kHasNormalsTexture;
      normal_texture = images[it->second.TextureIndex()];
    }
    if (const auto& it = mat.additionalValues.find("occlusionTexture");
        it != mat.additionalValues.cend()) {
      bits |= MetallicRoughnessBits::kHasOcclusionTexture;
      occlusion_texture = images[it->second.TextureIndex()];
    }
    if (const auto& it = mat.values.find("metallicRoughnessTexture");
        it != mat.values.cend()) {
      bits |= MetallicRoughnessBits::kHasMetallicRoughnessTexture;
      metallic_roughness_texture = images[it->second.TextureIndex()];
    }
    if (const auto& it = mat.additionalValues.find("emissiveTexture");
        it != mat.additionalValues.cend()) {
      bits |= MetallicRoughnessBits::kHashEmissiveTexture;
      emissive_texture = images[it->second.TextureIndex()];
    }
    MaterialInstance* material =
        material_cache->Get("Metallic Roughness", bits)->CreateInstance();
    material->SetParam(1, 5, data);
    if (base_color_texture) {
      material->SetTexture(1, 0, base_color_texture);
    }
    if (normal_texture) {
      material->SetTexture(1, 1, normal_texture);
    }
    if (occlusion_texture) {
      material->SetTexture(1, 2, occlusion_texture);
    }
    if (metallic_roughness_texture) {
      material->SetTexture(1, 3, metallic_roughness_texture);
    }
    if (emissive_texture) {
      material->SetTexture(1, 4, emissive_texture);
    }
    /*if (mat.additionalValues.find("emissiveTexture") !=
            mat.additionalValues.end()) {
          material.emissiveTexture =
              &textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
        }
        if (mat.additionalValues.find("alphaMode") !=
       mat.additionalValues.end()) { tinygltf::Parameter param =
       mat.additionalValues["alphaMode"]; if (param.string_value == "BLEND") {
            material.alphaMode = Material::ALPHAMODE_BLEND;
          }
          if (param.string_value == "MASK") {
            material.alphaCutoff = 0.5f;
            material.alphaMode = Material::ALPHAMODE_MASK;
          }
        }
        if (mat.additionalValues.find("alphaCutoff") !=
            mat.additionalValues.end()) {
          material.alphaCutoff =
              static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
        }
        if (mat.additionalValues.find("emissiveFactor") !=
            mat.additionalValues.end()) {
          material.emissiveFactor = glm::vec4(
              glm::make_vec3(
                  mat.additionalValues["emissiveFactor"].ColorFactor().data()),
              1.0);
          material.emissiveFactor = glm::vec4(0.0f);
        }*/
    materials.push_back(material);
  }

  for (const auto& gltf_mesh : gltf.meshes) {
    for (const auto& gltf_primitive : gltf_mesh.primitives) {
      assert(gltf_primitive.mode == TINYGLTF_MODE_TRIANGLES);

      std::vector<Vertex> vertices = VerticesFromGltf(gltf, gltf_primitive);
      assert(!vertices.empty());
      Primitive primitive;
      primitive.material = materials[gltf_primitive.material];
      primitive.index_buffer = IndexBufferFromGltf(
          device, command_pool, gltf, gltf_primitive, primitive.num_primitives);
      primitive.vertex_buffer = RenderAPI::CreateBuffer(
          device, RenderAPI::BufferUsageFlagBits::kVertexBuffer,
          sizeof(Vertex) * vertices.size(), RenderAPI::MemoryUsage::kGpu);
      RenderAPI::StageCopyDataToBuffer(command_pool, primitive.vertex_buffer,
                                       vertices.data(),
                                       sizeof(Vertex) * vertices.size());

      mesh.primitives.push_back(std::move(primitive));
    }
    break;
  }

  const tinygltf::Node& node = gltf.nodes[0];
  glm::mat4 mat = glm::mat4(1.0f);
  if (node.matrix.size() == 16) {
    mat = glm::make_mat4x4(gltf.nodes[0].matrix.data());
  } else {
    if (node.scale.size() == 3) {
      glm::scale(mat, glm::vec3(glm::make_vec3(node.scale.data())));
    }
    if (node.rotation.size() == 4) {
      glm::quat quat = glm::make_quat(node.rotation.data());
      mat = glm::mat4(quat) * mat;
    }
    if (node.translation.size() == 3) {
      glm::translate(mat, glm::vec3(glm::make_vec3(node.translation.data())));
    }
  }
  mesh.mat_world = mat;

  return true;
}