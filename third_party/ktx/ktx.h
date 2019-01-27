#pragma once

#include <cstdint>

namespace ktx {
static const char kKtxIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                        0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
struct KtxHeader {
  uint8_t identifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                            0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
  uint32_t endianness = 0x04030201;
  uint32_t gl_type;
  uint32_t gl_type_size = 1;
  uint32_t gl_format;
  uint32_t gl_internal_format;
  uint32_t gl_base_internal_format;
  uint32_t pixel_width;
  uint32_t pixel_height;
  uint32_t pixel_depth;
  uint32_t number_of_array_elements;
  uint32_t number_of_faces;
  uint32_t number_of_mipmap_levels;
  uint32_t bytes_of_key_value_data;
};

struct KtxKeyValuePair {
  uint32_t key_and_value_byte_size;
  const char* key;
  const char* value;
  const char* padding;
};

uint32_t CalculateKeyValuePairPadding(uint32_t key_and_value_byte_size);
uint32_t GetNumKeyValuePairs(const char* mem, uint32_t bytes_of_key_value_data);
bool GetKeyValuePair(uint32_t index, uint32_t bytes_of_key_value_data,
                     const char* mem, KtxKeyValuePair* out);
}  // namespace ktx