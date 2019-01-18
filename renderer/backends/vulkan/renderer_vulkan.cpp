#include "renderer_vulkan.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "detail/renderer_vulkan_detail.h"
#include "generational/generational_vector.h"

// Remove windows defined CreateSemaphore macro.
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

namespace Renderer {
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

  std::vector<VkAttachmentReference> color_attachment_refs(
      info.color_attachments.size());
  uint32_t idx = 0;
  for (const auto& it : info.color_attachments) {
    color_attachment_refs[idx].attachment = idx;
    color_attachment_refs[idx].layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ++idx;
  }

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = idx;
  subpass.pColorAttachments = color_attachment_refs.data();

  VkSubpassDependency dependency[2] = {};
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
  dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount =
      static_cast<uint32_t>(info.color_attachments.size());
  renderPassInfo.pAttachments =
      reinterpret_cast<const VkAttachmentDescription*>(
          info.color_attachments.data());
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependency;

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

PipelineLayout CreatePipelineLayout(
    Device device, const std::vector<DescriptorSetLayout>& info) {
  assert(sizeof(VkPipelineLayout) == sizeof(PipelineLayout));
  VkPipelineLayout layout;

  VkDescriptorSetLayout vk_layouts[256];
  assert(info.size() <= 256);
  for (size_t i = 0; i < info.size(); ++i) {
    vk_layouts[i] = descriptor_set_layouts_[info[i]].layout;
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = info.size();
  pipelineLayoutInfo.pSetLayouts = vk_layouts;       // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional
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

GraphicsPipeline CreateGraphicsPipeline(
    Device device_handle, RenderPass pass_handle,
    const GraphicsPipelineCreateInfo& info) {
  auto& device = devices_[device_handle];
  auto& pass = render_passes_[pass_handle];

  // Create the pipeline.
  GraphicsPipelineVk pipeline;
  pipeline.device = device_handle;

  VkShaderModule vertex = CreateShaderModule(device.device, info.vertex.code,
                                             info.vertex.code_size);
  VkShaderModule fragment = CreateShaderModule(
      device.device, info.fragment.code, info.fragment.code_size);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertex;
  vertShaderStageInfo.pName = "main";
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragment;
  fragShaderStageInfo.pName = "main";
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  // Create the vertex binding description.
  size_t num_vertex_attributes = 0;
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(
      info.vertex_input.size());
  uint32_t binding = 0;
  for (const auto& vertex_binding : info.vertex_input) {
    size_t binding_size = 0;
    num_vertex_attributes += vertex_binding.layout.size();
    for (const auto& vertex_attribute : vertex_binding.layout) {
      binding_size += GetVertexAttributeSize(vertex_attribute.type);
    }
    bindingDescriptions[binding].binding = binding;
    bindingDescriptions[binding].stride = binding_size;
    bindingDescriptions[binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    ++binding;
  }

  // Create the vertex attributes descriptions.
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(
      num_vertex_attributes);
  binding = 0;
  uint32_t location = 0;
  for (const auto& vertex_binding : info.vertex_input) {
    size_t binding_size = 0;
    for (const auto& vertex_attribute : vertex_binding.layout) {
      attributeDescriptions[location].binding = binding;
      attributeDescriptions[location].location = location;
      attributeDescriptions[location].format =
          GetFormatFromVertexAttributeType(vertex_attribute.type);
      attributeDescriptions[location].offset = vertex_attribute.offset;
      ++location;
    }
    ++binding;
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindingDescriptions.size());
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)1980;
  viewport.height = (float)1200;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = {1980, 1200};

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
  rasterizer.depthBiasClamp = 0.0f;           // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;           // Optional
  multisampling.pSampleMask = nullptr;             // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
  multisampling.alphaToOneEnable = VK_FALSE;       // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;  // Optional
  colorBlending.blendConstants[1] = 0.0f;  // Optional
  colorBlending.blendConstants[2] = 0.0f;  // Optional
  colorBlending.blendConstants[3] = 0.0f;  // Optional

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_LINE_WIDTH};

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;  // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;  // Optional
  pipelineInfo.layout = reinterpret_cast<VkPipelineLayout>(info.layout);
  pipelineInfo.renderPass = pass.pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
  pipelineInfo.basePipelineIndex = -1;               // Optional

  if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline.pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device.device, vertex, nullptr);
  vkDestroyShaderModule(device.device, fragment, nullptr);

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

void CmdBeginRenderPass(CommandBuffer buffer_handle, RenderPass pass_handle,
                        Framebuffer framebuffer_handle) {
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = render_passes_[pass_handle].pass;
  renderPassInfo.framebuffer = framebuffers_[framebuffer_handle].buffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {1920, 1200};
  VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;
  vkCmdBeginRenderPass(command_buffers_[buffer_handle].buffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void CmdEndRenderPass(CommandBuffer buffer_handle) {
  vkCmdEndRenderPass(command_buffers_[buffer_handle].buffer);
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
  assert(info.wait_semaphores_count <= 1);
  for (uint32_t i = 0; i < info.wait_semaphores_count; ++i) {
    waitSemaphores[i] = semaphores_[info.wait_semaphores[i]].semaphore;
  }
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.waitSemaphoreCount = info.wait_semaphores_count;

  VkCommandBuffer cmds[256] = {};
  for (uint32_t i = 0; i < info.command_buffers_count; ++i) {
    cmds[i] = command_buffers_[info.command_buffers[i]].buffer;
  }
  submitInfo.commandBufferCount = info.command_buffers_count;
  submitInfo.pCommandBuffers = cmds;

  VkSemaphore signalSemaphores[256] = {};
  assert(info.signal_semaphores_count <= 1);
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

Buffer CreateBuffer(Device device, BufferType type, uint64_t size,
                    MemoryUsage memory_usage) {
  auto& device_ref = devices_[device];
  BufferVk buffer;
  buffer.device = device;

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage =
      BufferUsageToVulkan(type) | MemoryUsageToVulkan(memory_usage);
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
  }
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vk_bindings.size());
  layoutInfo.pBindings = vk_bindings.data();

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

  assert(sizeof(VkDescriptorSet) == sizeof(DescriptorSet));

  if (vkAllocateDescriptorSets(pool_ref.device, &allocInfo,
                               reinterpret_cast<VkDescriptorSet*>(sets)) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
}

void UpdateDescriptorSets(Device device, uint32_t descriptor_write_count,
                          WriteDescriptorSet* descriptor_writes,
                          uint32_t descriptor_copy_count,
                          void* descriptor_copies) {
  VkDescriptorBufferInfo buffers[20];
  VkDescriptorImageInfo image_info[20];
  uint32_t buffer_offset = 0;
  uint32_t image_offset = 0;

  VkWriteDescriptorSet write_sets[10] = {};
  assert(descriptor_copy_count <= 10);
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
    if (descriptor_writes[i].type == DescriptorType::kUniformBuffer) {
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
                         write_sets, 0, nullptr);
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
  imageInfo.arrayLayers = 1;
  imageInfo.format = static_cast<VkFormat>(info.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0;

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
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

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
                          uint64_t size, const BufferImageCopy& info) {
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

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset.x = info.offset.x;
  region.imageOffset.y = info.offset.y;
  region.imageOffset.z = info.offset.z;
  region.imageExtent.width = info.extent.width;
  region.imageExtent.height = info.extent.height;
  region.imageExtent.depth = info.extent.depth;
  vkCmdCopyBufferToImage(cmd, buffer, image_ref.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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
  assert(sizeof(ImageView) == sizeof(VkImageView));
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

Sampler CreateSampler(Device device) {
  assert(sizeof(Sampler) == sizeof(VkSampler));

  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  // samplerInfo.magFilter = VK_FILTER_LINEAR;
  // samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
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
}  // namespace Renderer
