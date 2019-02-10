#pragma once

#include <Renderer/Material.h>
#include "model.h"
#include "renderer.h"

Mesh CreateCubeMesh(RenderAPI::Device device,
                    RenderAPI::CommandPool command_pool);
Mesh CreateSphereMesh(RenderAPI::Device device,
                      RenderAPI::CommandPool command_pool);
Mesh CreatePlaneMesh(RenderAPI::Device device,
                     RenderAPI::CommandPool command_pool);
bool MeshFromGLTF(RenderAPI::Device device, RenderAPI::CommandPool command_pool,
                  Renderer* renderer, Mesh& mesh);