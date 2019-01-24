#pragma once

#include <vector>

namespace Renderer {
using Bool32 = uint32_t;

enum class PrimitiveTopology {
  kPointList = 0,
  kLineList = 1,
  kLineStrip = 2,
  kTriangleList = 3,
  kTriangleStrip = 4,
  kTriangleFan = 5,
  kLineListWithAdjacency = 6,
  kLineStripWithAdjacency = 7,
  kTriangleListWithAdjacency = 8,
  kTriangleStripWithAdjacency = 9,
  kPatchList = 10,
};

struct PrimitiveInfo {
  PrimitiveTopology topology = PrimitiveTopology::kTriangleList;
  bool restart_enable = false;
};

enum class DynamicState {
  kViewport = 0,
  kScissor = 1,
  kLineWidth = 2,
  kDepthBias = 3,
  kBlendConstants = 4,
  kDepthBounds = 5,
  kStencilCompareMask = 6,
  kStencilWriteMask = 7,
  kStencilReference = 8,
  kViewportWScalingNV = 1000087000,
  kDiscardRectangleEXT = 1000099000,
  kSampleLocationsEXT = 1000143000,
  kViewportShadingRatePaletteNV = 1000164004,
  kViewportCoarseSampleOrderNV = 1000164006,
  kExclusiveScissorNV = 1000205001,
};

struct Viewport {
  float x = 0.0f;
  float y = 0.0f;
  float width;
  float height;
  float min_depth = 0.0f;
  float max_depth = 1.0f;

  Viewport(float x, float y, float width, float height, float min_depth = 0.0f,
           float max_depth = 1.0f)
      : x(x),
        y(y),
        width(width),
        height(height),
        min_depth(min_depth),
        max_depth(max_depth) {}
};

struct Extent2D {
  uint32_t width;
  uint32_t height;
  Extent2D() = default;
  Extent2D(uint32_t width, uint32_t height) : width(width), height(height) {}
};

struct Offset2D {
  uint32_t x;
  uint32_t y;
  Offset2D() = default;
  Offset2D(uint32_t x, uint32_t y) : x(x), y(y) {}
};

struct Rect2D {
  Offset2D offset;
  Extent2D extent;
};

struct ViewportState {
  std::vector<Viewport> viewports;
  std::vector<Rect2D> scissors;
};

enum class PolygonMode {
  kFill = 0,
  kLine = 1,
  kPoint = 2,
  kFillRectangleNV = 1000153000,
};

enum class FrontFace {
  kCounterClockwise = 0,
  kClockwise = 1,
};

namespace CullModeFlagBits {
enum CullModeFlagBits {
  kNone = 0,
  kFront = 0x00000001,
  kBack = 0x00000002,
  kFrontAndBack = 0x00000003,
};
}
using CullModeFlags = uint32_t;

struct RasterizationState {
  bool depth_clamp_enable = false;
  bool rasterizer_discard_enable = false;
  PolygonMode polygon_mode = PolygonMode::kFill;
  CullModeFlags cull_mode = CullModeFlagBits::kBack;
  FrontFace front_face = FrontFace::kCounterClockwise;
  bool depth_bias_enable = false;
  float depth_bias_constant_factor = 0.0f;
  float depth_bias_clamp = 0.0f;
  float depth_bias_slope_factor = 0.0f;
  float line_width = 1.0f;
};

enum class CompareOp {
  kNever = 0,
  kLess = 1,
  kEqual = 2,
  kLessOrEqual = 3,
  kGreater = 4,
  kNotEqual = 5,
  kGreaterOrEqual = 6,
  kAlways = 7,
};

enum class StencilOp {
  kKeep = 0,
  kZero = 1,
  kReplace = 2,
  kIncrementAndClamp = 3,
  kDecrementAndClamp = 4,
  kInvert = 5,
  kIncrementAndWrap = 6,
  kDecrementAndWrap = 7,
};

struct StencilOpState {
  StencilOp fail_op = StencilOp::kKeep;
  StencilOp pass_op = StencilOp::kKeep;
  StencilOp depth_fail_op = StencilOp::kKeep;
  CompareOp compare_op = CompareOp::kAlways;
  uint32_t compare_mask = 1;
  uint32_t write_mask = 1;
  uint32_t reference = 0;
};

struct DepthStencilState {
  bool depth_test_enable = false;
  bool depth_write_enable = false;
  CompareOp depth_compare_op = CompareOp::kLess;
  bool depth_bounds_test_enable = false;
  bool stencil_test_enable = false;
  StencilOpState front;
  StencilOpState back;
  float min_depth_bounds = 0.0f;
  float max_depth_bounds = 1.0f;
};

enum class LogicOp {
  kClear = 0,
  kAnd = 1,
  kAndReverse = 2,
  kCopy = 3,
  kAndInverted = 4,
  kNoOp = 5,
  kXor = 6,
  kOr = 7,
  kNor = 8,
  kEquivalent = 9,
  kInvert = 10,
  kOrReverse = 11,
  kCopyInverted = 12,
  kOrInverted = 13,
  kNand = 14,
  kSet = 15,
};

enum class BlendFactor {
  kZero = 0,
  kOne = 1,
  kSrcColor = 2,
  kOneMinusSrcColor = 3,
  kDstColor = 4,
  kOneMinusDstColor = 5,
  kSrcAlpha = 6,
  kOneMinusSrcAlpha = 7,
  kDstAlpha = 8,
  kOneMinusDstAlpha = 9,
  kConstantColor = 10,
  kOneMinusConstantColor = 11,
  kConstantAlpha = 12,
  kOneMinusConstantAlpha = 13,
  kAlphaSaturate = 14,
  kSrc1Color = 15,
  kOneMinusSrc1Color = 16,
  kSrc1Alpha = 17,
  kOneMinusSrc1Alpha = 18,
};

enum class BlendOp {
  kAdd = 0,
  kSubtract = 1,
  kReverseSubtract = 2,
  kMin = 3,
  kMax = 4,
  kZero = 1000148000,
  kSrc = 1000148001,
  kDst = 1000148002,
  kSrcOver = 1000148003,
  kDstOver = 1000148004,
  kSrcIn = 1000148005,
  kDstIn = 1000148006,
  kSrcOut = 1000148007,
  kDstOut = 1000148008,
  kSrcAtop = 1000148009,
  kDstAtop = 1000148010,
  kXor = 1000148011,
  kMultiply = 1000148012,
  kScreen = 1000148013,
  kOverlay = 1000148014,
  kDarken = 1000148015,
  kLighten = 1000148016,
  kColorDodge = 1000148017,
  kColorBurn = 1000148018,
  kHardLight = 1000148019,
  kSoftLight = 1000148020,
  kDifference = 1000148021,
  kExclusion = 1000148022,
  kInvert = 1000148023,
  kInvertRGB = 1000148024,
  kLinearDodge = 1000148025,
  kLinearBurn = 1000148026,
  kVividLight = 1000148027,
  kLinearLight = 1000148028,
  kPinLight = 1000148029,
  kHardMix = 1000148030,
  kHslHue = 1000148031,
  kHslSaturation = 1000148032,
  kHslColor = 1000148033,
  kHslLuminosity = 1000148034,
  kPlus = 1000148035,
  kPlusClamped = 1000148036,
  kPlusClampedAlpha = 1000148037,
  kPlusDarker = 1000148038,
  kMinus = 1000148039,
  kMinusClamped = 1000148040,
  kContrast = 1000148041,
  kInvertOVG = 1000148042,
  kRed = 1000148043,
  kGreen = 1000148044,
  kBlue = 1000148045,
};

namespace ColorComponentFlagBits {
enum ColorComponentFlagBits {
  kRed = 0x00000001,
  kGreen = 0x00000002,
  kBlue = 0x00000004,
  kAlpha = 0x00000008,
};
}  // namespace ColorComponentFlagBits

using ColorComponentFlags = uint32_t;

struct ColorBlendAttachmentState {
  Bool32 blend_enable = false;
  BlendFactor src_color_blend_factor = BlendFactor::kOne;
  BlendFactor dst_color_blend_factor = BlendFactor::kZero;
  BlendOp color_blend_op = BlendOp::kAdd;
  BlendFactor src_alpha_blend_factor = BlendFactor::kOne;
  BlendFactor dst_alpha_blend_factor = BlendFactor::kZero;
  BlendOp alpha_blend_op = BlendOp::kAdd;
  ColorComponentFlags color_write_mask =
      ColorComponentFlagBits::kRed | ColorComponentFlagBits::kGreen |
      ColorComponentFlagBits::kBlue | ColorComponentFlagBits::kAlpha;
};

struct ColorBlendState {
  bool logic_op_enable = false;
  LogicOp logic_op = LogicOp::kCopy;
  std::vector<ColorBlendAttachmentState> attachments;
  float blend_constants[4];
};

struct DynamicStateInfo {
  std::vector<DynamicState> states;
};

struct GraphicsPipelineStateInfo {
  PrimitiveInfo primitive;
  ViewportState viewport;
  RasterizationState rasterization;
  DepthStencilState depth_stencil;
  ColorBlendState blend;
  DynamicStateInfo dynamic_states;
};

}  // namespace Renderer