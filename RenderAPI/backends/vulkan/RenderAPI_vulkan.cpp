#include "RenderAPI_vulkan.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "detail/RenderAPI_vulkan_detail.h"
#include "generational/generational_vector.h"

// Remove windows defined CreateSemaphore macro.
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

namespace RenderAPI {
namespace {
GenerationalVector<InstanceVk> instances_;
GenerationalVector<DeviceVk> devices_;
GenerationalVector<SwapChainVk> swapchains_;
GenerationalVector<RenderPassVk> render_passes_;
GenerationalVector<GraphicsPipelineVk> graphic_pipelines_;
GenerationalVector<FramebufferVk> framebuffers_;
GenerationalVector<BufferVk> buffers_;
GenerationalVector<CommandPoolVk> command_pools_;
GenerationalVector<CommandBufferVk> command_buffers_;
GenerationalVector<SemaphoreVk> semaphores_;
GenerationalVector<FenceVk> fences_;
GenerationalVector<DescriptorSetLayoutVk> descriptor_set_layouts_;
GenerationalVector<DescriptorSetPoolVk> descriptor_set_pools_;
GenerationalVector<ImageVk> images_;
}  // namespace

Instance Create(const char* const* extensions, uint32_t extensions_count) {
  DebugAsserts();
  const std::vector<const char*> validation_layers = {
      "VK_LAYER_LUNARG_standard_validation"};
  std::vector<const char*> extensions_vector(extensions,
                                             extensions + extensions_count);

  if (enable_validation_layers &&
      !CheckValidationLayerSupport(validation_layers)) {
    throw std::runtime_error("Validation layers not supported!");
    return kInvalidHandle;
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "App Name";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Void Renderer";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  if (enable_validation_layers) {
    extensions_vector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    create_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(extensions_vector.size());
  create_info.ppEnabledExtensionNames = extensions_vector.data();

  InstanceVk instance;
  VkResult result = vkCreateInstance(&create_info, nullptr, &instance.instance);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance!");
    return kInvalidHandle;
  }

  if (enable_validation_layers) {
    CreateDebugMessenger(instance);
  }

  return instances_.Create(std::move(instance));
}

Device CreateDevice(Instance instance_handle) {
  InstanceVk& instance = instances_[instance_handle];
  VkPhysicalDevice physical_device = SelectPhysicalDevice(instance);
  QueueFamilyIndices device_indices =
      FindQueueFamilies(instance, physical_device);

  const std::vector<const char*> validation_layers = {
      "VK_LAYER_LUNARG_standard_validation"};
  DeviceVk device;
  device.instance = instance_handle;
  device.physical_device = physical_device;
  device.indices = device_indices;
  device.device =
      CreateLogicalDevice(physical_device, device_indices, validation_layers);
  vkGetDeviceQueue(device.device, device_indices.graphics, 0,
                   &device.graphics_queue);
  vkGetDeviceQueue(device.device, device_indices.presentation, 0,
                   &device.present_queue);

  // Create Vulkan Memory Allocator.
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = physical_device;
  allocatorInfo.device = device.device;
  vmaCreateAllocator(&allocatorInfo, &device.allocator);

  return devices_.Create(std::move(device));
}

void DeviceWaitIdle(Device device) {
  vkDeviceWaitIdle(devices_[device].device);
}

SwapChain CreateSwapChain(Device device_handle, uint32_t width,
                          uint32_t height) {
  auto& device = devices_[device_handle];
  auto& instance = instances_[device.instance];
  SwapChainSupportDetails swapChainSupport =
      QuerySwapChainSupport(instance.surface, device.physical_device);

  VkSurfaceFormatKHR surfaceFormat =
      ChooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      ChooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent =
      ChooseSwapExtent(width, height, swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = instance.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  uint32_t queueFamilyIndices[] = {
      static_cast<uint32_t>(device.indices.graphics),
      static_cast<uint32_t>(device.indices.presentation)};

  if (device.indices.graphics != device.indices.presentation) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;      // Optional
    createInfo.pQueueFamilyIndices = nullptr;  // Optional
  }
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  SwapChainVk swapchain;
  swapchain.device = device_handle;
  if (vkCreateSwapchainKHR(device.device, &createInfo, nullptr,
                           &swapchain.swap_chain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
    return kInvalidHandle;
  }

  vkGetSwapchainImagesKHR(device.device, swapchain.swap_chain, &imageCount,
                          nullptr);
  swapchain.images.resize(imageCount);
  vkGetSwapchainImagesKHR(device.device, swapchain.swap_chain, &imageCount,
                          swapchain.images.data());
  swapchain.format = surfaceFormat.format;
  swapchain.extent = extent;
  CreateSwapChainImageViews(device, swapchain);
  return swapchains_.Create(std::move(swapchain));
}

void DestroySwapChain(SwapChain swapchain_handle) {
  auto& swapchain = swapchains_[swapchain_handle];
  auto& device = devices_[swapchain.device];
  for (auto image_view : swapchain.image_views) {
    vkDestroyImageView(device.device, image_view, nullptr);
  }
  vkDestroySwapchainKHR(device.device, swapchain.swap_chain, nullptr);
}

uint32_t GetSwapChainLength(SwapChain swapchain_handle) {
  auto& swapchain = swapchains_[swapchain_handle];
  return static_cast<uint32_t>(swapchain.image_views.size());
}

TextureFormat GetSwapChainImageFormat(SwapChain swapchain) {
  return static_cast<TextureFormat>(swapchains_[swapchain].format);
}

Framebuffer CreateFramebuffer(Device device,
                              const FramebufferCreateInfo& info) {
  VkFramebufferCreateInfo framebufferInfo = {};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  auto& pass = render_passes_[info.pass];
  framebufferInfo.renderPass = pass.pass;
  framebufferInfo.attachmentCount =
      static_cast<uint32_t>(info.attachments.size());
  framebufferInfo.pAttachments =
      reinterpret_cast<const VkImageView*>(info.attachments.data());
  framebufferInfo.width = info.width;
  framebufferInfo.height = info.height;
  framebufferInfo.layers = 1;

  FramebufferVk buffer;
  buffer.device = device;
  if (vkCreateFramebuffer(devices_[device].device, &framebufferInfo, nullptr,
                          &buffer.buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create framebuffer!");
  }
  return framebuffers_.Create(std::move(buffer));
}

ImageView GetSwapChainImageView(SwapChain swapchain, uint32_t index) {
  auto& swapchain_ref = swapchains_[swapchain];
  assert(index < swapchain_ref.image_views.size());
  return reinterpret_cast<ImageView>(swapchain_ref.image_views[index]);
}

void AcquireNextImage(SwapChain swapchain_handle, uint64_t timeout_ns,
                      Semaphore semaphore, uint32_t* out_image_index) {
  auto& swapchain = swapchains_[swapchain_handle];
  auto& device = devices_[swapchain.device];
  VkSemaphore vk_semaphore = (semaphore != kInvalidHandle)
                                 ? semaphores_[semaphore].semaphore
                                 : VK_NULL_HANDLE;
  vkAcquireNextImageKHR(device.device, swapchain.swap_chain, timeout_ns,
                        vk_semaphore, VK_NULL_HANDLE, out_image_index);
}

void DestroyDevice(Device device_handle) {
  auto& device = devices_[device_handle];
  vmaDestroyAllocator(device.allocator);
  vkDestroyDevice(device.device, nullptr);
}

void Destroy(Instance instance_handle) {
  auto& instance = instances_[instance_handle];
  vkDestroySurfaceKHR(instance.instance, instance.surface, nullptr);
  if (enable_validation_layers) {
    DestroyDebugUtilsMessengerEXT(instance.instance, instance.debug_callback,
                                  nullptr);
  }
  vkDestroyInstance(instance.instance, nullptr);
}

RenderPass CreateRenderPass(Device device_handle,
                            const RenderPassCreateInfo& info) {
  auto& device = devices_[device_handle];

  RenderPassVk pass;
  pass.device = device_handle;

  std::vector<VkAttachmentReference>
      color_attachment_refs;  //(info.color_attachments.size());
  std::vector<VkAttachmentReference>
      depth_stencil_attachment_refs;  //(info.depth_stencil_attachments.size());
  uint32_t idx = 0;
  for (const auto& it : info.attachments) {
    VkAttachmentReference attachment;
    attachment.attachment = idx;
    attachment.layout = IsDepthStencilFormat(it.format)
                            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (attachment.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      depth_stencil_attachment_refs.emplace_back(std::move(attachment));
    } else {
      color_attachment_refs.emplace_back(std::move(attachment));
    }
    ++idx;
  }

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = color_attachment_refs.size();
  subpass.pColorAttachments = color_attachment_refs.data();
  subpass.pDepthStencilAttachment = depth_stencil_attachment_refs.data();

  /*VkSubpassDependency dependency[2] = {};
  dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency[0].dstSubpass = 0;
  dependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependency[1].srcSubpass = 0;
  dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;*/

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount =
      static_cast<uint32_t>(info.attachments.size());
  renderPassInfo.pAttachments =
      reinterpret_cast<const VkAttachmentDescription*>(info.attachments.data());
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 0;
  renderPassInfo.pDependencies = nullptr;

  if (vkCreateRenderPass(device.device, &renderPassInfo, nullptr, &pass.pass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }

  return render_passes_.Create(std::move(pass));
}

void DestroyRenderPass(RenderPass pass_handle) {
  auto& pass = render_passes_[pass_handle];
  auto& device = devices_[pass.device];
  vkDestroyRenderPass(device.device, pass.pass, nullptr);
  render_passes_.Destroy(pass_handle);
}

PipelineLayout CreatePipelineLayout(Device device,
                                    const PipelineLayoutCreateInfo& info) {
  VkPipelineLayout layout;

  VkDescriptorSetLayout vk_layouts[256];
  assert(info.layouts.size() <= 256);
  for (size_t i = 0; i < info.layouts.size(); ++i) {
    vk_layouts[i] = descriptor_set_layouts_[info.layouts[i]].layout;
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = info.layouts.size();
  pipelineLayoutInfo.pSetLayouts = vk_layouts;
  pipelineLayoutInfo.pushConstantRangeCount = info.push_constants.size();
  pipelineLayoutInfo.pPushConstantRanges =
      reinterpret_cast<const VkPushConstantRange*>(info.push_constants.data());
  if (vkCreatePipelineLayout(devices_[device].device, &pipelineLayoutInfo,
                             nullptr, &layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
  return reinterpret_cast<PipelineLayout>(layout);
}

void DestroyPipelineLayout(Device device, PipelineLayout layout) {
  vkDestroyPipelineLayout(devices_[device].device,
                          reinterpret_cast<VkPipelineLayout>(layout), nullptr);
}

ShaderModule CreateShaderModule(Device device, const uint32_t* code,
                                size_t size) {
  return reinterpret_cast<ShaderModule>(
      CreateShaderModule(devices_[device].device, code, size));
}
void DestroyShaderModule(Device device, ShaderModule module) {
  vkDestroyShaderModule(devices_[device].device,
                        reinterpret_cast<VkShaderModule>(module), nullptr);
}

GraphicsPipeline CreateGraphicsPipeline(
    Device device_handle, RenderPass pass_handle,
    const GraphicsPipelineCreateInfo& info) {
  assert((std::find(info.states.dynamic_states.states.cbegin(),
                    info.states.dynamic_states.states.cend(),
                    RenderAPI::DynamicState::kViewport) !=
              info.states.dynamic_states.states.cend() ||
          !info.states.viewport.viewports.empty()) &&
         "Must define a valid viewport!");

  auto& device = devices_[device_handle];
  auto& pass = render_passes_[pass_handle];

  // Create the pipeline.
  GraphicsPipelineVk pipeline;
  pipeline.device = device_handle;

  VkShaderModule vertex =
      (info.vertex.module == VK_NULL_HANDLE)
          ? CreateShaderModule(device.device, info.vertex.code,
                               info.vertex.code_size)
          : reinterpret_cast<VkShaderModule>(info.vertex.module);
  VkShaderModule fragment =
      (info.fragment.module == VK_NULL_HANDLE)
          ? CreateShaderModule(device.device, info.fragment.code,
                               info.fragment.code_size)
          : reinterpret_cast<VkShaderModule>(info.fragment.module);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertex;
  vertShaderStageInfo.pName = "main";
  vertShaderStageInfo.pSpecializationInfo =
      reinterpret_cast<const VkSpecializationInfo*>(info.vertex.specialization);
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragment;
  fragShaderStageInfo.pName = "main";
  fragShaderStageInfo.pSpecializationInfo =
      reinterpret_cast<const VkSpecializationInfo*>(
          info.fragment.specialization);
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  // Create the vertex binding description.
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(info.vertex_input.bindings.size());
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(info.vertex_input.attributes.size());
  vertexInputInfo.pVertexBindingDescriptions =
      reinterpret_cast<const VkVertexInputBindingDescription*>(
          info.vertex_input.bindings.data());
  vertexInputInfo.pVertexAttributeDescriptions =
      reinterpret_cast<const VkVertexInputAttributeDescription*>(
          info.vertex_input.attributes.data());

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology =
      static_cast<VkPrimitiveTopology>(info.states.primitive.topology);
  inputAssembly.primitiveRestartEnable = info.states.primitive.restart_enable;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount =
      static_cast<uint32_t>(info.states.viewport.viewports.size());
  viewportState.pViewports = reinterpret_cast<const VkViewport*>(
      info.states.viewport.viewports.data());
  if (info.states.viewport.scissors.empty() &&
      !info.states.viewport.viewports.empty()) {
    scissor.extent.width = info.states.viewport.viewports[0].width;
    scissor.extent.height = info.states.viewport.viewports[0].height;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
  } else {
    viewportState.scissorCount =
        static_cast<uint32_t>(info.states.viewport.scissors.size());
    viewportState.pScissors =
        reinterpret_cast<const VkRect2D*>(info.states.viewport.scissors.data());
  }

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = info.states.rasterization.depth_clamp_enable;
  rasterizer.rasterizerDiscardEnable =
      info.states.rasterization.rasterizer_discard_enable;
  rasterizer.polygonMode =
      static_cast<VkPolygonMode>(info.states.rasterization.polygon_mode);
  rasterizer.lineWidth = info.states.rasterization.line_width;
  rasterizer.cullMode =
      static_cast<VkCullModeFlags>(info.states.rasterization.cull_mode);
  rasterizer.frontFace =
      static_cast<VkFrontFace>(info.states.rasterization.front_face);
  rasterizer.depthBiasEnable = info.states.rasterization.depth_bias_enable;
  rasterizer.depthBiasConstantFactor =
      info.states.rasterization.depth_bias_constant_factor;  // Optional
  rasterizer.depthBiasClamp =
      info.states.rasterization.depth_bias_clamp;  // Optional
  rasterizer.depthBiasSlopeFactor =
      info.states.rasterization.depth_bias_slope_factor;  // Optional

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;           // Optional
  multisampling.pSampleMask = nullptr;             // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
  multisampling.alphaToOneEnable = VK_FALSE;       // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = info.states.blend.logic_op_enable;
  colorBlending.logicOp =
      static_cast<VkLogicOp>(info.states.blend.logic_op);  // Optional
  colorBlending.attachmentCount =
      static_cast<uint32_t>(info.states.blend.attachments.size());
  colorBlending.pAttachments =
      reinterpret_cast<const VkPipelineColorBlendAttachmentState*>(
          info.states.blend.attachments.data());
  colorBlending.blendConstants[0] =
      info.states.blend.blend_constants[0];  // Optional
  colorBlending.blendConstants[1] =
      info.states.blend.blend_constants[1];  // Optional
  colorBlending.blendConstants[2] =
      info.states.blend.blend_constants[2];  // Optional
  colorBlending.blendConstants[3] =
      info.states.blend.blend_constants[3];  // Optional

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount =
      static_cast<uint32_t>(info.states.dynamic_states.states.size());
  dynamicState.pDynamicStates = reinterpret_cast<const VkDynamicState*>(
      info.states.dynamic_states.states.data());

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
  depth_stencil_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_state.pNext = nullptr;
  depth_stencil_state.flags = 0;
  depth_stencil_state.depthTestEnable =
      info.states.depth_stencil.depth_test_enable;
  depth_stencil_state.depthWriteEnable =
      info.states.depth_stencil.depth_write_enable;
  depth_stencil_state.depthCompareOp =
      static_cast<VkCompareOp>(info.states.depth_stencil.depth_compare_op);
  depth_stencil_state.depthBoundsTestEnable =
      info.states.depth_stencil.depth_bounds_test_enable;
  depth_stencil_state.stencilTestEnable =
      info.states.depth_stencil.stencil_test_enable;
  memcpy(&depth_stencil_state.front, &info.states.depth_stencil.front,
         sizeof(VkStencilOpState));
  memcpy(&depth_stencil_state.back, &info.states.depth_stencil.back,
         sizeof(VkStencilOpState));
  depth_stencil_state.minDepthBounds =
      info.states.depth_stencil.min_depth_bounds;
  depth_stencil_state.maxDepthBounds =
      info.states.depth_stencil.max_depth_bounds;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depth_stencil_state;  // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;  // Optional
  pipelineInfo.layout = reinterpret_cast<VkPipelineLayout>(info.layout);
  pipelineInfo.renderPass = pass.pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
  pipelineInfo.basePipelineIndex = -1;               // Optional

  if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline.pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  if (info.vertex.module == VK_NULL_HANDLE) {
    vkDestroyShaderModule(device.device, vertex, nullptr);
  }
  if (info.fragment.module == VK_NULL_HANDLE) {
    vkDestroyShaderModule(device.device, fragment, nullptr);
  }

  GraphicsPipeline handle = graphic_pipelines_.Create(std::move(pipeline));
  return handle;
}

void DestroyGraphicsPipeline(GraphicsPipeline pipeline_handle) {
  auto& pipeline = graphic_pipelines_[pipeline_handle];
  auto& device = devices_[pipeline.device];
  vkDestroyPipeline(device.device, pipeline.pipeline, nullptr);
  graphic_pipelines_.Destroy(pipeline_handle);
}

void DestroyFramebuffer(Framebuffer buffer_handle) {
  auto& buffer = framebuffers_[buffer_handle];
  vkDestroyFramebuffer(devices_[buffer.device].device, buffer.buffer, nullptr);
  framebuffers_.Destroy(buffer_handle);
}

CommandPool CreateCommandPool(Device device_handle,
                              CommandPoolCreateFlags flags) {
  CommandPoolVk pool;
  pool.device = device_handle;
  auto& device = devices_[device_handle];

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = device.indices.graphics;
  poolInfo.flags = static_cast<VkCommandPoolCreateFlagBits>(flags);
  if (vkCreateCommandPool(device.device, &poolInfo, nullptr, &pool.pool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
  return command_pools_.Create(std::move(pool));
}

void DestroyCommandPool(CommandPool pool_handle) {
  auto& pool = command_pools_[pool_handle];
  vkDestroyCommandPool(devices_[pool.device].device, pool.pool, nullptr);
  command_pools_.Destroy(pool_handle);
}

CommandBuffer CreateCommandBuffer(CommandPool pool_handle) {
  auto& pool = command_pools_[pool_handle];
  auto& device = devices_[pool.device];

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool.pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  CommandBufferVk buffer;
  buffer.device = pool.device;
  buffer.pool = pool_handle;
  if (vkAllocateCommandBuffers(device.device, &allocInfo, &buffer.buffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
  return command_buffers_.Create(std::move(buffer));
}

void DestroyCommandBuffer(CommandBuffer buffer_handle) {
  auto& buffer = command_buffers_[buffer_handle];
  auto& device = devices_[buffer.device];
  auto& pool = command_pools_[buffer.pool];
  vkFreeCommandBuffers(device.device, pool.pool, 1, &buffer.buffer);
  command_buffers_.Destroy(buffer_handle);
}

void CmdBegin(CommandBuffer buffer_handle) {
  auto& buffer = command_buffers_[buffer_handle];
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;  // Optional

  if (vkBeginCommandBuffer(buffer.buffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void CmdEnd(CommandBuffer buffer_handle) {
  auto& buffer = command_buffers_[buffer_handle];
  if (vkEndCommandBuffer(buffer.buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void CmdBeginRenderPass(CommandBuffer buffer_handle,
                        const BeginRenderPassInfo& info) {
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = render_passes_[info.pass].pass;
  renderPassInfo.framebuffer = framebuffers_[info.framebuffer].buffer;
  renderPassInfo.renderArea =
      *reinterpret_cast<const VkRect2D*>(&info.render_area);
  renderPassInfo.clearValueCount = info.clear_values_count;
  renderPassInfo.pClearValues =
      reinterpret_cast<const VkClearValue*>(info.clear_values);
  vkCmdBeginRenderPass(command_buffers_[buffer_handle].buffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void CmdEndRenderPass(CommandBuffer buffer_handle) {
  vkCmdEndRenderPass(command_buffers_[buffer_handle].buffer);
}

void CmdSetViewport(CommandBuffer buffer, uint32_t first_viewport,
                    uint32_t viewport_count, const Viewport* viewports) {
  vkCmdSetViewport(command_buffers_[buffer].buffer, first_viewport,
                   viewport_count,
                   reinterpret_cast<const VkViewport*>(viewports));
}

void CmdSetScissor(CommandBuffer buffer, uint32_t first_scissor, uint32_t count,
                   const Rect2D* scissors) {
  vkCmdSetScissor(command_buffers_[buffer].buffer, first_scissor, count,
                  reinterpret_cast<const VkRect2D*>(scissors));
}

void CmdBindPipeline(CommandBuffer buffer_handle,
                     GraphicsPipeline pipeline_handle) {
  vkCmdBindPipeline(command_buffers_[buffer_handle].buffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphic_pipelines_[pipeline_handle].pipeline);
}

void CmdBindVertexBuffers(CommandBuffer cmd, uint32_t first_binding,
                          uint32_t binding_count, const Buffer* buffers,
                          const uint64_t* offsets) {
  static constexpr uint64_t kOffsets[256] = {0};
  assert(binding_count <= 256);
  if (!offsets) {
    offsets = kOffsets;
  }
  VkBuffer vk_buffers[256];
  for (uint32_t i = 0; i < binding_count; ++i) {
    vk_buffers[i] = buffers_[buffers[i]].buffer;
  }
  vkCmdBindVertexBuffers(command_buffers_[cmd].buffer, first_binding,
                         binding_count, vk_buffers, offsets);
}

void CmdBindIndexBuffer(CommandBuffer cmd, Buffer buffer, IndexType type,
                        uint64_t offset) {
  vkCmdBindIndexBuffer(command_buffers_[cmd].buffer, buffers_[buffer].buffer,
                       offset, static_cast<VkIndexType>(type));
}

void CmdBindDescriptorSets(CommandBuffer cmd, int type, PipelineLayout layout,
                           uint32_t first, uint32_t count,
                           const DescriptorSet* sets,
                           uint32_t dynamic_sets_count,
                           const uint32_t* dynamic_sets_offsets) {
  vkCmdBindDescriptorSets(command_buffers_[cmd].buffer,
                          static_cast<VkPipelineBindPoint>(type),
                          reinterpret_cast<VkPipelineLayout>(layout), first,
                          count, reinterpret_cast<const VkDescriptorSet*>(sets),
                          dynamic_sets_count, dynamic_sets_offsets);
}

void CmdPushConstants(CommandBuffer cmd, PipelineLayout layout,
                      ShaderStageFlags flags, uint32_t offset, uint32_t size,
                      const void* values) {
  vkCmdPushConstants(command_buffers_[cmd].buffer,
                     reinterpret_cast<VkPipelineLayout>(layout), flags, offset,
                     size, values);
}

void CmdDraw(CommandBuffer buffer, uint32_t vertex_count,
             uint32_t instance_count, uint32_t first_vertex,
             uint32_t first_instance) {
  vkCmdDraw(command_buffers_[buffer].buffer, vertex_count, instance_count,
            first_vertex, first_instance);
}

void CmdDrawIndexed(CommandBuffer cmd, uint32_t index_count,
                    uint32_t instance_count, uint32_t first_index,
                    int32_t vertex_offset, uint32_t first_instance) {
  vkCmdDrawIndexed(command_buffers_[cmd].buffer, index_count, instance_count,
                   first_index, vertex_offset, first_instance);
}

void CmdCopyBuffer(CommandBuffer cmd, Buffer src, Buffer dst,
                   uint32_t region_count, const BufferCopy* regions) {
  vkCmdCopyBuffer(command_buffers_[cmd].buffer, buffers_[src].buffer,
                  buffers_[dst].buffer, region_count,
                  reinterpret_cast<const VkBufferCopy*>(regions));
}

Semaphore CreateSemaphore(Device device_handle) {
  auto& device = devices_[device_handle];
  SemaphoreVk semaphore;
  semaphore.device = device.device;
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(device.device, &semaphoreInfo, nullptr,
                        &semaphore.semaphore) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
  }
  return semaphores_.Create(std::move(semaphore));
}

void DestroySemaphore(Semaphore handle) {
  auto& semaphore = semaphores_[handle];
  vkDestroySemaphore(semaphore.device, semaphore.semaphore, nullptr);
  semaphores_.Destroy(handle);
}

Fence CreateFence(Device device, bool signaled) {
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = (signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);

  FenceVk fence;
  fence.device = devices_[device].device;
  if (vkCreateFence(fence.device, &fenceInfo, nullptr, &fence.fence) !=
      VK_SUCCESS) {
    throw std::runtime_error(
        "failed to create synchronization objects for a frame!");
  }
  return fences_.Create(std::move(fence));
}

void DestroyFence(Fence fence) {
  auto& fence_ref = fences_[fence];
  vkDestroyFence(fence_ref.device, fence_ref.fence, nullptr);
}

void WaitForFences(const Fence* fences, uint32_t count, bool wait_for_all,
                   uint64_t timeout_ns) {
  VkFence vk_fences[256];
  assert(count <= 256);
  for (uint32_t i = 0; i < count; ++i) {
    vk_fences[i] = fences_[fences[i]].fence;
  }
  vkWaitForFences(fences_[fences[0]].device, count, vk_fences, wait_for_all,
                  timeout_ns);
}
void ResetFences(const Fence* fences, uint32_t count) {
  VkFence vk_fences[256];
  assert(count <= 256);
  for (uint32_t i = 0; i < count; ++i) {
    vk_fences[i] = fences_[fences[i]].fence;
  }
  vkResetFences(fences_[fences[0]].device, count, vk_fences);
}

void QueueSubmit(Device device, const SubmitInfo& info, Fence fence) {
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[256] = {};
  std::vector<VkPipelineStageFlags> waitStages(
      info.wait_semaphores_count,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  assert(info.wait_semaphores_count <= 256);
  for (uint32_t i = 0; i < info.wait_semaphores_count; ++i) {
    waitSemaphores[i] = semaphores_[info.wait_semaphores[i]].semaphore;
  }

  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.waitSemaphoreCount = info.wait_semaphores_count;

  VkCommandBuffer cmds[256] = {};
  for (uint32_t i = 0; i < info.command_buffers_count; ++i) {
    cmds[i] = command_buffers_[info.command_buffers[i]].buffer;
  }
  submitInfo.commandBufferCount = info.command_buffers_count;
  submitInfo.pCommandBuffers = cmds;

  VkSemaphore signalSemaphores[256] = {};
  assert(info.signal_semaphores_count <= 256);
  for (uint32_t i = 0; i < info.signal_semaphores_count; ++i) {
    signalSemaphores[i] = semaphores_[info.signal_semaphores[i]].semaphore;
  }
  submitInfo.signalSemaphoreCount = info.signal_semaphores_count;
  submitInfo.pSignalSemaphores = signalSemaphores;

  const VkFence vk_fence =
      (fence == kInvalidHandle) ? VK_NULL_HANDLE : fences_[fence].fence;
  if (vkQueueSubmit(devices_[device].graphics_queue, 1, &submitInfo,
                    vk_fence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
}

void QueuePresent(SwapChain swapchain_handle, uint32_t image,
                  const PresentInfo& info) {
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  VkSemaphore waitSemaphores[256] = {};
  assert(info.wait_semaphores_count <= 1);
  for (uint32_t i = 0; i < info.wait_semaphores_count; ++i) {
    waitSemaphores[i] = semaphores_[info.wait_semaphores[i]].semaphore;
  }
  presentInfo.waitSemaphoreCount = info.wait_semaphores_count;
  presentInfo.pWaitSemaphores = waitSemaphores;

  auto& swapchain = swapchains_[swapchain_handle];
  VkSwapchainKHR swapChains[] = {swapchain.swap_chain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &image;
  presentInfo.pResults = nullptr;  // Optional
  vkQueuePresentKHR(devices_[swapchain.device].present_queue, &presentInfo);
}

Buffer CreateBuffer(Device device, BufferUsageFlags usage, uint64_t size,
                    MemoryUsage memory_usage) {
  auto& device_ref = devices_[device];
  BufferVk buffer;
  buffer.device = device;

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = usage | MemoryUsageToVulkan(memory_usage);
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = MemoryUsageToVulkanMemoryAllocator(memory_usage);
  allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  vmaCreateBuffer(device_ref.allocator, &bufferInfo, &allocInfo, &buffer.buffer,
                  &buffer.allocation, nullptr);

  return buffers_.Create(std::move(buffer));
}

void DestroyBuffer(Buffer buffer) {
  auto& buffer_ref = buffers_[buffer];
  auto& device_ref = devices_[buffer_ref.device];

  vmaDestroyBuffer(device_ref.allocator, buffer_ref.buffer,
                   buffer_ref.allocation);

  buffers_.Destroy(buffer);
}

void* MapBuffer(Buffer buffer) {
  auto& buffer_ref = buffers_[buffer];
  auto& device_ref = devices_[buffer_ref.device];

  void* mappedData;
  vmaMapMemory(device_ref.allocator, buffer_ref.allocation, &mappedData);
  return mappedData;
}

void UnmapBuffer(Buffer buffer) {
  auto& buffer_ref = buffers_[buffer];
  auto& device_ref = devices_[buffer_ref.device];
  vmaUnmapMemory(device_ref.allocator, buffer_ref.allocation);
}

DescriptorSetLayout CreateDescriptorSetLayout(
    Device device, const DescriptorSetLayoutCreateInfo& info) {
  DescriptorSetLayoutVk layout;
  layout.device = devices_[device].device;

  assert(info.bindings.size() <= 20);
  VkDescriptorBindingFlagsEXT binding_flag_bits_ext[20] = {0};

  std::vector<VkDescriptorSetLayoutBinding> vk_bindings(info.bindings.size());
  for (uint32_t binding = 0;
       binding < static_cast<uint32_t>(info.bindings.size()); ++binding) {
    VkDescriptorSetLayoutBinding& vk_binding = vk_bindings[binding];
    const auto& info_binding = info.bindings[binding];
    vk_binding.binding = binding;
    vk_binding.descriptorType =
        static_cast<VkDescriptorType>(info_binding.type);
    vk_binding.descriptorCount = info_binding.count;
    vk_binding.stageFlags =
        static_cast<VkShaderStageFlags>(info_binding.stages);
    vk_binding.pImmutableSamplers = nullptr;
    binding_flag_bits_ext[binding] = info_binding.flags;
  }

  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_ext = {};
  binding_ext.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  binding_ext.bindingCount = static_cast<uint32_t>(vk_bindings.size());
  binding_ext.pBindingFlags = binding_flag_bits_ext;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vk_bindings.size());
  layoutInfo.pBindings = vk_bindings.data();
  layoutInfo.pNext = &binding_ext;

  if (vkCreateDescriptorSetLayout(layout.device, &layoutInfo, nullptr,
                                  &layout.layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
  return descriptor_set_layouts_.Create(std::move(layout));
}

void DestroyDescriptorSetLayout(DescriptorSetLayout layout) {
  auto& ref = descriptor_set_layouts_[layout];
  vkDestroyDescriptorSetLayout(ref.device, ref.layout, nullptr);
  descriptor_set_layouts_.Destroy(layout);
}

DescriptorSetPool CreateDescriptorSetPool(
    Device device, const CreateDescriptorSetPoolCreateInfo& info) {
  DescriptorSetPoolVk pool;
  pool.device = devices_[device].device;

  // Create the Vulkan pool.
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(info.pools.size());
  poolInfo.pPoolSizes =
      reinterpret_cast<const VkDescriptorPoolSize*>(info.pools.data());
  poolInfo.maxSets = info.max_sets;
  if (vkCreateDescriptorPool(pool.device, &poolInfo, nullptr, &pool.pool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  return descriptor_set_pools_.Create(std::move(pool));
}

void DestroyDescriptorSetPool(DescriptorSetPool pool) {
  auto& ref = descriptor_set_pools_[pool];
  vkDestroyDescriptorPool(ref.device, ref.pool, nullptr);
  descriptor_set_pools_.Destroy(pool);
}

void AllocateDescriptorSets(DescriptorSetPool pool,
                            const std::vector<DescriptorSetLayout>& layouts,
                            DescriptorSet* sets) {
  // Copy the vulkan layouts.
  std::vector<VkDescriptorSetLayout> vk_layouts(layouts.size());
  for (size_t i = 0; i < layouts.size(); ++i) {
    vk_layouts[i] = descriptor_set_layouts_[layouts[i]].layout;
  }

  // Create the vulkan set.
  auto& pool_ref = descriptor_set_pools_[pool];
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool_ref.pool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(vk_layouts.size());
  allocInfo.pSetLayouts = vk_layouts.data();

  if (vkAllocateDescriptorSets(pool_ref.device, &allocInfo,
                               reinterpret_cast<VkDescriptorSet*>(sets)) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}

void UpdateDescriptorSets(Device device, uint32_t descriptor_write_count,
                          WriteDescriptorSet* descriptor_writes,
                          uint32_t descriptor_copy_count,
                          CopyDescriptorSet* descriptor_copies) {
  VkDescriptorBufferInfo buffers[20];
  VkDescriptorImageInfo image_info[20];
  uint32_t buffer_offset = 0;
  uint32_t image_offset = 0;

  VkCopyDescriptorSet copies[10] = {};
  assert(descriptor_copy_count <= 10);
  for (uint32_t i = 0; i < descriptor_copy_count; ++i) {
    copies[i].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copies[i].pNext = nullptr;
    copies[i].srcSet =
        reinterpret_cast<VkDescriptorSet>(descriptor_copies[i].src_set);
    copies[i].srcBinding = descriptor_copies[i].src_binding;
    copies[i].srcArrayElement = descriptor_copies[i].src_array_element;
    copies[i].dstSet =
        reinterpret_cast<VkDescriptorSet>(descriptor_copies[i].dst_set);
    copies[i].dstBinding = descriptor_copies[i].dst_binding;
    copies[i].dstArrayElement = descriptor_copies[i].dst_array_element;
    copies[i].descriptorCount = descriptor_copies[i].descriptor_count;
  }

  VkWriteDescriptorSet write_sets[10] = {};
  assert(descriptor_write_count <= 10);
  for (uint32_t i = 0; i < descriptor_write_count; ++i) {
    write_sets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_sets[i].dstSet =
        reinterpret_cast<VkDescriptorSet>(descriptor_writes[i].set);
    write_sets[i].dstBinding = descriptor_writes[i].binding;
    write_sets[i].dstArrayElement = descriptor_writes[i].dst_array_element;
    write_sets[i].descriptorType =
        static_cast<VkDescriptorType>(descriptor_writes[i].type);
    write_sets[i].descriptorCount = descriptor_writes[i].descriptor_count;
    write_sets[i].pImageInfo = nullptr;
    write_sets[i].pTexelBufferView = nullptr;
    write_sets[i].pBufferInfo = nullptr;
    if (descriptor_writes[i].type == DescriptorType::kUniformBuffer ||
        descriptor_writes[i].type == DescriptorType::kStorageBuffer) {
      write_sets[i].pBufferInfo = &buffers[buffer_offset];
      for (uint32_t j = 0; j < descriptor_writes[i].descriptor_count; ++j) {
        assert(buffer_offset < 20);
        const auto& buffer = descriptor_writes[i].buffers[j];
        buffers[buffer_offset].buffer = buffers_[buffer.buffer].buffer;
        buffers[buffer_offset].offset = buffer.offset;
        buffers[buffer_offset].range = buffer.range;
        ++buffer_offset;
      }
    } else if (descriptor_writes[i].type ==
               DescriptorType::kCombinedImageSampler) {
      write_sets[i].pImageInfo = &image_info[image_offset];
      for (uint32_t j = 0; j < descriptor_writes[i].descriptor_count; ++j) {
        assert(image_offset < 20);
        const auto& image = descriptor_writes[i].images[j];
        image_info[image_offset].imageView =
            reinterpret_cast<VkImageView>(image.image_view);
        image_info[image_offset].sampler =
            reinterpret_cast<VkSampler>(image.sampler);
        image_info[image_offset].imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        ++image_offset;
      }
    } else {
      assert(false);
    }
  }
  vkUpdateDescriptorSets(devices_[device].device, descriptor_write_count,
                         write_sets, descriptor_copy_count, copies);
}

Image CreateImage(Device device, const ImageCreateInfo& info) {
  auto& device_ref = devices_[device];
  ImageVk image;
  image.device = device;

  VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = static_cast<VkImageType>(info.type);
  imageInfo.extent.width = info.extent.width;
  imageInfo.extent.height = info.extent.height;
  imageInfo.extent.depth = info.extent.depth;
  imageInfo.mipLevels = info.mips;
  imageInfo.arrayLayers = info.array_layers;
  imageInfo.format = static_cast<VkFormat>(info.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = info.usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = info.flags;

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  if (vmaCreateImage(device_ref.allocator, &imageInfo, &imageAllocCreateInfo,
                     &image.image, &image.allocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image!");
  }
  return images_.Create(std::move(image));
}

void DestroyImage(Image image) {
  auto& ref = images_[image];
  auto& device_ref = devices_[ref.device];
  vmaDestroyImage(device_ref.allocator, ref.image, ref.allocation);
  images_.Destroy(image);
}

VkCommandBuffer BeginSingleTimeCommands(DeviceVk& device, VkCommandPool pool) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void EndSingleTimeCommands(DeviceVk& device, VkCommandPool pool,
                           VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = 0;

  VkFence fence;
  if (vkCreateFence(device.device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create fence!");
  }

  vkQueueSubmit(device.graphics_queue, 1, &submitInfo, fence);
  vkWaitForFences(device.device, 1, &fence, true,
                  std::numeric_limits<uint64_t>::max());

  vkDestroyFence(device.device, fence, nullptr);
  vkFreeCommandBuffers(device.device, pool, 1, &commandBuffer);
}

void RecordTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

void StageCopyDataToImage(CommandPool pool, Image image, const void* data,
                          uint64_t size, uint32_t num_regions,
                          const BufferImageCopy* regions) {
  auto& pool_ref = command_pools_[pool];
  auto& device_ref = devices_[pool_ref.device];
  auto& image_ref = images_[image];

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VkBuffer buffer;
  VmaAllocation allocation;
  vmaCreateBuffer(device_ref.allocator, &bufferInfo, &allocInfo, &buffer,
                  &allocation, nullptr);
  void* mapped_data;
  vmaMapMemory(device_ref.allocator, allocation, &mapped_data);
  memcpy(mapped_data, data, static_cast<size_t>(size));
  vmaUnmapMemory(device_ref.allocator, allocation);

  VkCommandBuffer cmd = BeginSingleTimeCommands(device_ref, pool_ref.pool);
  RecordTransitionImageLayout(cmd, image_ref.image, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkCmdCopyBufferToImage(cmd, buffer, image_ref.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_regions,
                         reinterpret_cast<const VkBufferImageCopy*>(regions));

  RecordTransitionImageLayout(cmd, image_ref.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  EndSingleTimeCommands(device_ref, pool_ref.pool, cmd);
  vmaDestroyBuffer(device_ref.allocator, buffer, allocation);
}

void StageCopyDataToBuffer(CommandPool pool, Buffer buffer, const void* data,
                           uint64_t size) {
  auto& pool_ref = command_pools_[pool];
  auto& device_ref = devices_[pool_ref.device];

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VkBuffer staging_buffer;
  VmaAllocation allocation;
  vmaCreateBuffer(device_ref.allocator, &bufferInfo, &allocInfo,
                  &staging_buffer, &allocation, nullptr);
  void* mapped_data;
  vmaMapMemory(device_ref.allocator, allocation, &mapped_data);
  memcpy(mapped_data, data, static_cast<size_t>(size));
  vmaUnmapMemory(device_ref.allocator, allocation);

  VkCommandBuffer cmd = BeginSingleTimeCommands(device_ref, pool_ref.pool);
  VkBufferCopy copyRegion = {};
  copyRegion.size = size;
  vkCmdCopyBuffer(cmd, staging_buffer, buffers_[buffer].buffer, 1, &copyRegion);
  EndSingleTimeCommands(device_ref, pool_ref.pool, cmd);
  vmaDestroyBuffer(device_ref.allocator, staging_buffer, allocation);
}

ImageView CreateImageView(Device device, const ImageViewCreateInfo& info) {
  auto& device_ref = devices_[device];

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = images_[info.image].image;
  viewInfo.viewType = static_cast<VkImageViewType>(info.type);
  viewInfo.format = static_cast<VkFormat>(info.format);
  viewInfo.components.r = static_cast<VkComponentSwizzle>(info.swizzle.r);
  viewInfo.components.g = static_cast<VkComponentSwizzle>(info.swizzle.g);
  viewInfo.components.b = static_cast<VkComponentSwizzle>(info.swizzle.b);
  viewInfo.components.a = static_cast<VkComponentSwizzle>(info.swizzle.a);
  viewInfo.subresourceRange.aspectMask =
      static_cast<VkImageAspectFlags>(info.subresource_range.aspect_mask);
  viewInfo.subresourceRange.baseMipLevel =
      info.subresource_range.base_mipmap_level;
  viewInfo.subresourceRange.levelCount = info.subresource_range.level_count;
  viewInfo.subresourceRange.baseArrayLayer =
      info.subresource_range.base_array_layer;
  viewInfo.subresourceRange.layerCount = info.subresource_range.layer_count;

  VkImageView imageView;
  if (vkCreateImageView(device_ref.device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  return reinterpret_cast<ImageView>(imageView);
}

void DestroyImageView(Device device, ImageView view) {
  vkDestroyImageView(devices_[device].device,
                     reinterpret_cast<VkImageView>(view), nullptr);
}

Sampler CreateSampler(Device device, SamplerCreateInfo info) {
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = static_cast<VkFilter>(info.mag_filter);
  samplerInfo.minFilter = static_cast<VkFilter>(info.min_filter);
  samplerInfo.addressModeU =
      static_cast<VkSamplerAddressMode>(info.address_mode_u);
  samplerInfo.addressModeV =
      static_cast<VkSamplerAddressMode>(info.address_mode_v);
  samplerInfo.addressModeW =
      static_cast<VkSamplerAddressMode>(info.address_mode_w);
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = info.compare_enable;
  samplerInfo.compareOp = static_cast<VkCompareOp>(info.compare_op);
  samplerInfo.mipmapMode = static_cast<VkSamplerMipmapMode>(info.mipmap_mode);
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = info.min_lod;
  samplerInfo.maxLod = info.max_lod;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1;

  VkSampler sampler;
  if (vkCreateSampler(devices_[device].device, &samplerInfo, nullptr,
                      &sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
  return reinterpret_cast<Sampler>(sampler);
}

void DestroySampler(Device device, Sampler sampler) {
  vkDestroySampler(devices_[device].device,
                   reinterpret_cast<VkSampler>(sampler), nullptr);
}

// TODO: Find a more elegant way to solve the surface problem?
void SetSurface(Instance instance, VkSurfaceKHR surface) {
  instances_[instance].surface = surface;
}

VkInstance GetVkInstance(Instance instance) {
  return instances_[instance].instance;
}
}  // namespace RenderAPI
