#pragma once

#include <RenderUtils/TextureManager.h>
#include <Renderer/Material.h>
#include <vector>
#include "MaterialCache.h"
#include "model.h"
#include "renderer.h"

Mesh CreateCubeMesh(RenderAPI::Device device,
                    RenderAPI::CommandPool command_pool);
Mesh CreateSphereMesh(RenderAPI::Device device,
                      RenderAPI::CommandPool command_pool);
Mesh CreatePlaneMesh(RenderAPI::Device device,
                     RenderAPI::CommandPool command_pool);
bool MeshFromGLTF(RenderAPI::Device device, RenderAPI::CommandPool command_pool,
                  MaterialCache* material_cache, Mesh& mesh,
                  RenderUtils::TextureManager* texture_manager);