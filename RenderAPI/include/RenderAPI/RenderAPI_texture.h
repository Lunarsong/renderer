#pragma once

#include <cstdint>

namespace RenderAPI {
struct Extent3D {
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  Extent3D() = default;
  Extent3D(uint32_t width, uint32_t height, uint32_t depth)
      : width(width), height(height), depth(depth) {}
};
struct Offset3D {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  Offset3D() = default;
  Offset3D(uint32_t x, uint32_t y, uint32_t z) : x(x), y(y), z(z) {}
};

enum class TextureType {
  Texture1D = 0,
  Texture2D = 1,
  Texture3D = 2,
};

enum class ImageViewType {
  Texture1D = 0,
  Texture2D = 1,
  Texture3D = 2,
  Cube,
  Array1D,
  Array2D,
};

enum class TextureFormat {
  kUndefined = 0,
  kR4G4_UNORM_PACK8 = 1,
  kR4G4B4A4_UNORM_PACK16 = 2,
  kB4G4R4A4_UNORM_PACK16 = 3,
  kR5G6B5_UNORM_PACK16 = 4,
  kB5G6R5_UNORM_PACK16 = 5,
  kR5G5B5A1_UNORM_PACK16 = 6,
  kB5G5R5A1_UNORM_PACK16 = 7,
  kA1R5G5B5_UNORM_PACK16 = 8,
  kR8_UNORM = 9,
  kR8_SNORM = 10,
  kR8_USCALED = 11,
  kR8_SSCALED = 12,
  kR8_UINT = 13,
  kR8_SINT = 14,
  kR8_SRGB = 15,
  kR8G8_UNORM = 16,
  kR8G8_SNORM = 17,
  kR8G8_USCALED = 18,
  kR8G8_SSCALED = 19,
  kR8G8_UINT = 20,
  kR8G8_SINT = 21,
  kR8G8_SRGB = 22,
  kR8G8B8_UNORM = 23,
  kR8G8B8_SNORM = 24,
  kR8G8B8_USCALED = 25,
  kR8G8B8_SSCALED = 26,
  kR8G8B8_UINT = 27,
  kR8G8B8_SINT = 28,
  kR8G8B8_SRGB = 29,
  kB8G8R8_UNORM = 30,
  kB8G8R8_SNORM = 31,
  kB8G8R8_USCALED = 32,
  kB8G8R8_SSCALED = 33,
  kB8G8R8_UINT = 34,
  kB8G8R8_SINT = 35,
  kB8G8R8_SRGB = 36,
  kR8G8B8A8_UNORM = 37,
  kR8G8B8A8_SNORM = 38,
  kR8G8B8A8_USCALED = 39,
  kR8G8B8A8_SSCALED = 40,
  kR8G8B8A8_UINT = 41,
  kR8G8B8A8_SINT = 42,
  kR8G8B8A8_SRGB = 43,
  kB8G8R8A8_UNORM = 44,
  kB8G8R8A8_SNORM = 45,
  kB8G8R8A8_USCALED = 46,
  kB8G8R8A8_SSCALED = 47,
  kB8G8R8A8_UINT = 48,
  kB8G8R8A8_SINT = 49,
  kB8G8R8A8_SRGB = 50,
  kA8B8G8R8_UNORM_PACK32 = 51,
  kA8B8G8R8_SNORM_PACK32 = 52,
  kA8B8G8R8_USCALED_PACK32 = 53,
  kA8B8G8R8_SSCALED_PACK32 = 54,
  kA8B8G8R8_UINT_PACK32 = 55,
  kA8B8G8R8_SINT_PACK32 = 56,
  kA8B8G8R8_SRGB_PACK32 = 57,
  kA2R10G10B10_UNORM_PACK32 = 58,
  kA2R10G10B10_SNORM_PACK32 = 59,
  kA2R10G10B10_USCALED_PACK32 = 60,
  kA2R10G10B10_SSCALED_PACK32 = 61,
  kA2R10G10B10_UINT_PACK32 = 62,
  kA2R10G10B10_SINT_PACK32 = 63,
  kA2B10G10R10_UNORM_PACK32 = 64,
  kA2B10G10R10_SNORM_PACK32 = 65,
  kA2B10G10R10_USCALED_PACK32 = 66,
  kA2B10G10R10_SSCALED_PACK32 = 67,
  kA2B10G10R10_UINT_PACK32 = 68,
  kA2B10G10R10_SINT_PACK32 = 69,
  kR16_UNORM = 70,
  kR16_SNORM = 71,
  kR16_USCALED = 72,
  kR16_SSCALED = 73,
  kR16_UINT = 74,
  kR16_SINT = 75,
  kR16_SFLOAT = 76,
  kR16G16_UNORM = 77,
  kR16G16_SNORM = 78,
  kR16G16_USCALED = 79,
  kR16G16_SSCALED = 80,
  kR16G16_UINT = 81,
  kR16G16_SINT = 82,
  kR16G16_SFLOAT = 83,
  kR16G16B16_UNORM = 84,
  kR16G16B16_SNORM = 85,
  kR16G16B16_USCALED = 86,
  kR16G16B16_SSCALED = 87,
  kR16G16B16_UINT = 88,
  kR16G16B16_SINT = 89,
  kR16G16B16_SFLOAT = 90,
  kR16G16B16A16_UNORM = 91,
  kR16G16B16A16_SNORM = 92,
  kR16G16B16A16_USCALED = 93,
  kR16G16B16A16_SSCALED = 94,
  kR16G16B16A16_UINT = 95,
  kR16G16B16A16_SINT = 96,
  kR16G16B16A16_SFLOAT = 97,
  kR32_UINT = 98,
  kR32_SINT = 99,
  kR32_SFLOAT = 100,
  kR32G32_UINT = 101,
  kR32G32_SINT = 102,
  kR32G32_SFLOAT = 103,
  kR32G32B32_UINT = 104,
  kR32G32B32_SINT = 105,
  kR32G32B32_SFLOAT = 106,
  kR32G32B32A32_UINT = 107,
  kR32G32B32A32_SINT = 108,
  kR32G32B32A32_SFLOAT = 109,
  kR64_UINT = 110,
  kR64_SINT = 111,
  kR64_SFLOAT = 112,
  kR64G64_UINT = 113,
  kR64G64_SINT = 114,
  kR64G64_SFLOAT = 115,
  kR64G64B64_UINT = 116,
  kR64G64B64_SINT = 117,
  kR64G64B64_SFLOAT = 118,
  kR64G64B64A64_UINT = 119,
  kR64G64B64A64_SINT = 120,
  kR64G64B64A64_SFLOAT = 121,
  kB10G11R11_UFLOAT_PACK32 = 122,
  kE5B9G9R9_UFLOAT_PACK32 = 123,
  kD16_UNORM = 124,
  kX8_D24_UNORM_PACK32 = 125,
  kD32_SFLOAT = 126,
  kS8_UINT = 127,
  kD16_UNORM_S8_UINT = 128,
  kD24_UNORM_S8_UINT = 129,
  kD32_SFLOAT_S8_UINT = 130,
  kBC1_RGB_UNORM_BLOCK = 131,
  kBC1_RGB_SRGB_BLOCK = 132,
  kBC1_RGBA_UNORM_BLOCK = 133,
  kBC1_RGBA_SRGB_BLOCK = 134,
  kBC2_UNORM_BLOCK = 135,
  kBC2_SRGB_BLOCK = 136,
  kBC3_UNORM_BLOCK = 137,
  kBC3_SRGB_BLOCK = 138,
  kBC4_UNORM_BLOCK = 139,
  kBC4_SNORM_BLOCK = 140,
  kBC5_UNORM_BLOCK = 141,
  kBC5_SNORM_BLOCK = 142,
  kBC6H_UFLOAT_BLOCK = 143,
  kBC6H_SFLOAT_BLOCK = 144,
  kBC7_UNORM_BLOCK = 145,
  kBC7_SRGB_BLOCK = 146,
  kETC2_R8G8B8_UNORM_BLOCK = 147,
  kETC2_R8G8B8_SRGB_BLOCK = 148,
  kETC2_R8G8B8A1_UNORM_BLOCK = 149,
  kETC2_R8G8B8A1_SRGB_BLOCK = 150,
  kETC2_R8G8B8A8_UNORM_BLOCK = 151,
  kETC2_R8G8B8A8_SRGB_BLOCK = 152,
  kEAC_R11_UNORM_BLOCK = 153,
  kEAC_R11_SNORM_BLOCK = 154,
  kEAC_R11G11_UNORM_BLOCK = 155,
  kEAC_R11G11_SNORM_BLOCK = 156,
  kASTC_4x4_UNORM_BLOCK = 157,
  kASTC_4x4_SRGB_BLOCK = 158,
  kASTC_5x4_UNORM_BLOCK = 159,
  kASTC_5x4_SRGB_BLOCK = 160,
  kASTC_5x5_UNORM_BLOCK = 161,
  kASTC_5x5_SRGB_BLOCK = 162,
  kASTC_6x5_UNORM_BLOCK = 163,
  kASTC_6x5_SRGB_BLOCK = 164,
  kASTC_6x6_UNORM_BLOCK = 165,
  kASTC_6x6_SRGB_BLOCK = 166,
  kASTC_8x5_UNORM_BLOCK = 167,
  kASTC_8x5_SRGB_BLOCK = 168,
  kASTC_8x6_UNORM_BLOCK = 169,
  kASTC_8x6_SRGB_BLOCK = 170,
  kASTC_8x8_UNORM_BLOCK = 171,
  kASTC_8x8_SRGB_BLOCK = 172,
  kASTC_10x5_UNORM_BLOCK = 173,
  kASTC_10x5_SRGB_BLOCK = 174,
  kASTC_10x6_UNORM_BLOCK = 175,
  kASTC_10x6_SRGB_BLOCK = 176,
  kASTC_10x8_UNORM_BLOCK = 177,
  kASTC_10x8_SRGB_BLOCK = 178,
  kASTC_10x10_UNORM_BLOCK = 179,
  kASTC_10x10_SRGB_BLOCK = 180,
  kASTC_12x10_UNORM_BLOCK = 181,
  kASTC_12x10_SRGB_BLOCK = 182,
  kASTC_12x12_UNORM_BLOCK = 183,
  kASTC_12x12_SRGB_BLOCK = 184,
  kG8B8G8R8_422_UNORM = 1000156000,
  kB8G8R8G8_422_UNORM = 1000156001,
  kG8_B8_R8_3PLANE_420_UNORM = 1000156002,
  kG8_B8R8_2PLANE_420_UNORM = 1000156003,
  kG8_B8_R8_3PLANE_422_UNORM = 1000156004,
  kG8_B8R8_2PLANE_422_UNORM = 1000156005,
  kG8_B8_R8_3PLANE_444_UNORM = 1000156006,
  kR10X6_UNORM_PACK16 = 1000156007,
  kR10X6G10X6_UNORM_2PACK16 = 1000156008,
  kR10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,
  kG10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
  kB10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
  kG10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
  kG10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
  kG10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
  kG10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
  kG10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
  kR12X4_UNORM_PACK16 = 1000156017,
  kR12X4G12X4_UNORM_2PACK16 = 1000156018,
  kR12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
  kG12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
  kB12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
  kG12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
  kG12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
  kG12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
  kG12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
  kG12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
  kG16B16G16R16_422_UNORM = 1000156027,
  kB16G16R16G16_422_UNORM = 1000156028,
  kG16_B16_R16_3PLANE_420_UNORM = 1000156029,
  kG16_B16R16_2PLANE_420_UNORM = 1000156030,
  kG16_B16_R16_3PLANE_422_UNORM = 1000156031,
  kG16_B16R16_2PLANE_422_UNORM = 1000156032,
  kG16_B16_R16_3PLANE_444_UNORM = 1000156033,
  kPVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
  kPVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
  kPVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
  kPVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
  kPVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
  kPVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
  kPVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
  kPVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
  kG8B8G8R8_422_UNORM_KHR = kG8B8G8R8_422_UNORM,
  kB8G8R8G8_422_UNORM_KHR = kB8G8R8G8_422_UNORM,
  kG8_B8_R8_3PLANE_420_UNORM_KHR = kG8_B8_R8_3PLANE_420_UNORM,
  kG8_B8R8_2PLANE_420_UNORM_KHR = kG8_B8R8_2PLANE_420_UNORM,
  kG8_B8_R8_3PLANE_422_UNORM_KHR = kG8_B8_R8_3PLANE_422_UNORM,
  kG8_B8R8_2PLANE_422_UNORM_KHR = kG8_B8R8_2PLANE_422_UNORM,
  kG8_B8_R8_3PLANE_444_UNORM_KHR = kG8_B8_R8_3PLANE_444_UNORM,
  kR10X6_UNORM_PACK16_KHR = kR10X6_UNORM_PACK16,
  kR10X6G10X6_UNORM_2PACK16_KHR = kR10X6G10X6_UNORM_2PACK16,
  kR10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = kR10X6G10X6B10X6A10X6_UNORM_4PACK16,
  kG10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR =
      kG10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
  kB10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR =
      kB10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
  kG10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR =
      kG10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
  kG10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR =
      kG10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
  kG10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR =
      kG10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
  kG10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR =
      kG10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
  kG10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR =
      kG10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
  kR12X4_UNORM_PACK16_KHR = kR12X4_UNORM_PACK16,
  kR12X4G12X4_UNORM_2PACK16_KHR = kR12X4G12X4_UNORM_2PACK16,
  kR12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR = kR12X4G12X4B12X4A12X4_UNORM_4PACK16,
  kG12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR =
      kG12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
  kB12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR =
      kB12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
  kG12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR =
      kG12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
  kG12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR =
      kG12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
  kG12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR =
      kG12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
  kG12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR =
      kG12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
  kG12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR =
      kG12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
  kG16B16G16R16_422_UNORM_KHR = kG16B16G16R16_422_UNORM,
  kB16G16R16G16_422_UNORM_KHR = kB16G16R16G16_422_UNORM,
  kG16_B16_R16_3PLANE_420_UNORM_KHR = kG16_B16_R16_3PLANE_420_UNORM,
  kG16_B16R16_2PLANE_420_UNORM_KHR = kG16_B16R16_2PLANE_420_UNORM,
  kG16_B16_R16_3PLANE_422_UNORM_KHR = kG16_B16_R16_3PLANE_422_UNORM,
  kG16_B16R16_2PLANE_422_UNORM_KHR = kG16_B16R16_2PLANE_422_UNORM,
  kG16_B16_R16_3PLANE_444_UNORM_KHR = kG16_B16_R16_3PLANE_444_UNORM,
};

enum class ComponentSwizzle {
  kIdentity = 0,
  kZero = 1,
  kOne = 2,
  kRed = 3,
  kGreen = 4,
  kBlue = 5,
  kAlpha = 6
};

struct Swizzle {
  ComponentSwizzle r = ComponentSwizzle::kIdentity;
  ComponentSwizzle g = ComponentSwizzle::kIdentity;
  ComponentSwizzle b = ComponentSwizzle::kIdentity;
  ComponentSwizzle a = ComponentSwizzle::kIdentity;
};

enum ImageUsageFlagBits {
  kTransferSrcBit = 0x00000001,
  kTransferDstBit = 0x00000002,
  kSampledBit = 0x00000004,
  kStorageBit = 0x00000008,
  kColorAttachmentBit = 0x00000010,
  kDepthStencilAttachmentBit = 0x00000020,
  kTransientAttachmentBit = 0x00000040,
  kinputAttachmentBit = 0x00000080,
  kMax = 0x7FFFFFFF
};
using ImageUsageFlags = uint32_t;

namespace ImageCreateFlagBits {
enum ImageCreateFlagBits {
  kSparseBindingBit = 0x00000001,
  kSparseResidencyBit = 0x00000002,
  kSParseAliasedBit = 0x00000004,
  kMutableFormatBit = 0x00000008,
  kCubeCompatibleBit = 0x00000010,
  kAliasBit = 0x00000400,
  kSplitInstanceBindRegionsBit = 0x00000040,
  k2DArrayCompatibleBit = 0x00000020,
  kBlockTexelViewCompatibleBit = 0x00000080,
  kExtendedUsageBit = 0x00000100,
  kProtectedBit = 0x00000800,
  kDisjointBit = 0x00000200,
  kCornerSampledBitNV = 0x00002000,
  kSampleLocationsCompatibleDepthBitEXT = 0x00001000,
  kSubsampledBitEXT = 0x00004000,
};
}  // namespace ImageCreateFlagBits
using ImageCreateFlags = uint32_t;

struct ImageCreateInfo {
  TextureType type;
  TextureFormat format;
  Extent3D extent;
  ImageUsageFlags usage =
      ImageUsageFlagBits::kTransferDstBit | ImageUsageFlagBits::kSampledBit;
  uint32_t mips = 1;
  uint32_t array_layers = 1;
  ImageCreateFlags flags = 0;

  ImageCreateInfo() = default;
  ImageCreateInfo(TextureType type, TextureFormat format, Extent3D extent,
                  ImageUsageFlags usage = ImageUsageFlagBits::kTransferDstBit |
                                          ImageUsageFlagBits::kSampledBit,
                  uint32_t mips = 1, uint32_t array_layers = 1)
      : type(type),
        format(format),
        extent(extent),
        usage(usage),
        mips(mips),
        array_layers(array_layers) {}
};

enum TextureCubeFaces {
  kTextureCubeRight,
  kTextureCubeLeft,
  kTextureCubeTop,
  kTextureCubeBottom,
  kTextureCubeBack,
  kTextureCubeFront,
  kTextureCubeSize
};

enum ImageAspectFlagBits {
  kColorBit = 0x00000001,
  kDepthBit = 0x00000002,
  kStencilBit = 0x00000004,
  kAspectMetadataBit = 0x00000008,
  kAspectPlane0Bit = 0x00000010,
  kAspectPlane1Bit = 0x00000020,
  kAspectPlane2Bit = 0x00000040,
  kAspectMemoryPlane0Bit_EXT = 0x00000080,
  kAspectMemoryPlane1Bit_EXT = 0x00000100,
  kAspectMemoryPlane2Bit_EXT = 0x00000200,
  kAspectMemoryPlane3Bit_EXT = 0x00000400,
  kAspectPlane0Bit_KHR = kAspectPlane0Bit,
  kAspectPlane1Bit_KHR = kAspectPlane1Bit,
  kAspectPlane2Bit_KHR = kAspectPlane2Bit,
};
using ImageAspectFlags = uint32_t;

constexpr uint32_t kRemainingMipLevels = ~0U;
constexpr uint32_t kRemainingArrayLayers = ~0U;

struct ImageSubresourceRange {
  ImageAspectFlags aspect_mask;
  uint32_t base_mipmap_level = 0;
  uint32_t level_count = kRemainingMipLevels;
  uint32_t base_array_layer = 0;
  uint32_t layer_count = kRemainingArrayLayers;

  ImageSubresourceRange() = default;
  ImageSubresourceRange(ImageAspectFlags aspect_mask,
                        uint32_t base_mipmap_level = 0,
                        uint32_t level_count = kRemainingMipLevels,
                        uint32_t base_array_layer = 0,
                        uint32_t layer_count = kRemainingArrayLayers)
      : aspect_mask(aspect_mask),
        base_mipmap_level(base_mipmap_level),
        level_count(level_count),
        base_array_layer(base_array_layer),
        layer_count(layer_count) {}
};

struct ImageSubresourceLayers {
  ImageAspectFlags aspect_mask = ImageAspectFlagBits::kColorBit;
  uint32_t mip_level = 0;
  uint32_t base_array_layer = 0;
  uint32_t layer_count = 1;
};

bool IsDepthStencilFormat(TextureFormat format);

enum class SamplerMipmapMode {
  kNearest = 0,
  kLinear = 1,
};

enum class SamplerAddressMode {
  kRepeat = 0,
  kMirroredRepeat = 1,
  kClampToEdge = 2,
  kClampToBorder = 3,
  kMirrorClampToEdge = 4,
};

enum class SamplerFilter {
  kNearest = 0,
  kLinear = 1,
};

struct SamplerCreateInfo {
  SamplerFilter min_filter = SamplerFilter::kNearest;
  SamplerFilter mag_filter = SamplerFilter::kNearest;
  SamplerMipmapMode mipmap_mode = SamplerMipmapMode::kLinear;
  SamplerAddressMode address_mode_u = SamplerAddressMode::kClampToEdge;
  SamplerAddressMode address_mode_v = SamplerAddressMode::kClampToEdge;
  SamplerAddressMode address_mode_w = SamplerAddressMode::kClampToEdge;
  float min_lod = 0.0f;
  float max_lod = 1.0f;
  bool compare_enable = false;
  CompareOp compare_op = CompareOp::kAlways;

  SamplerCreateInfo() = default;
  SamplerCreateInfo(
      SamplerFilter min_filter, SamplerFilter mag_filter,
      SamplerMipmapMode mipmap_mode = SamplerMipmapMode::kLinear,
      SamplerAddressMode address_mode_u = SamplerAddressMode::kClampToEdge,
      SamplerAddressMode address_mode_v = SamplerAddressMode::kClampToEdge,
      SamplerAddressMode address_mode_w = SamplerAddressMode::kClampToEdge)
      : min_filter(min_filter),
        mag_filter(mag_filter),
        mipmap_mode(mipmap_mode),
        address_mode_u(address_mode_u),
        address_mode_v(address_mode_v),
        address_mode_w(address_mode_w) {}
};

}  // namespace RenderAPI