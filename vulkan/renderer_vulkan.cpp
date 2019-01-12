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

Framebuffer CreateSwapChainFramebuffer(SwapChain swapchain_handle,
                                       uint32_t index, RenderPass pass_handle) {
  auto& swapchain = swapchains_[swapchain_handle];
  assert(index < swapchain.image_views.size());
  VkImageView attachments[] = {swapchain.image_views[index]};

  VkFramebufferCreateInfo framebufferInfo = {};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  auto& pass = render_passes_[pass_handle];
  framebufferInfo.renderPass = pass.pass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = attachments;
  framebufferInfo.width = swapchain.extent.width;
  framebufferInfo.height = swapchain.extent.height;
  framebufferInfo.layers = 1;

  FramebufferVk buffer;
  buffer.device = swapchain.device;
  if (vkCreateFramebuffer(devices_[swapchain.device].device, &framebufferInfo,
                          nullptr, &buffer.buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create framebuffer!");
  }
  return framebuffers_.Create(std::move(buffer));
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

RenderPass CreateRenderPass(Device device_handle, SwapChain swapchain_handle) {
  auto& device = devices_[device_handle];
  auto& swapchain = swapchains_[swapchain_handle];

  RenderPassVk pass;
  pass.device = device_handle;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = swapchain.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  colorAttachment.flags = 0;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

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
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
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

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;             // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr;          // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional
  if (vkCreatePipelineLayout(device.device, &pipelineLayoutInfo, nullptr,
                             &pipeline.layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

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
  pipelineInfo.layout = pipeline.layout;
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
  vkDestroyPipelineLayout(device.device, pipeline.layout, nullptr);
  graphic_pipelines_.Destroy(pipeline_handle);
}

void DestroyFramebuffer(Framebuffer buffer_handle) {
  auto& buffer = framebuffers_[buffer_handle];
  vkDestroyFramebuffer(devices_[buffer.device].device, buffer.buffer, nullptr);
  framebuffers_.Destroy(buffer_handle);
}

CommandPool CreateCommandPool(Device device_handle) {
  CommandPoolVk pool;
  pool.device = device_handle;
  auto& device = devices_[device_handle];

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = device.indices.graphics;
  poolInfo.flags = 0;  // Optional
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

Buffer CreateBuffer(Device device, BufferType type, uint64_t size) {
  auto& device_ref = devices_[device];
  BufferVk buffer;
  buffer.device = device;

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = BufferUsageToVulkan(type);
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;  // VMA_MEMORY_USAGE_GPU_ONLY;
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

// TODO: Find a more elegant way to solve the surface problem?
void SetSurface(Instance instance, VkSurfaceKHR surface) {
  instances_[instance].surface = surface;
}

VkInstance GetVkInstance(Instance instance) {
  return instances_[instance].instance;
}
}  // namespace Renderer
