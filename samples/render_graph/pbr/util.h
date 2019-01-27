#pragma once

#include "model.h"

Model CreateCubeModel(RenderAPI::Device device,
                      RenderAPI::CommandPool command_pool);
Model CreateSphereModel(RenderAPI::Device device,
                        RenderAPI::CommandPool command_pool);