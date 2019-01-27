#include "ktx.h"

#include <cstring>

namespace ktx {
uint32_t CalculateKeyValuePairPadding(uint32_t key_and_value_byte_size) {
  return 3 - ((key_and_value_byte_size + 3) % 4);
}

uint32_t GetNumKeyValuePairs(const char* mem,
                             uint32_t bytes_of_key_value_data) {
  uint32_t num_key_value_pairs = 0;

  uint32_t bytes_read = 0;
  while (bytes_read < bytes_of_key_value_data) {
    uint32_t key_and_value_byte_size =
        *reinterpret_cast<const uint32_t*>(mem + bytes_read);
    bytes_read += key_and_value_byte_size +
                  CalculateKeyValuePairPadding(key_and_value_byte_size);
    ++num_key_value_pairs;
  }
  return num_key_value_pairs;
}

bool GetKeyValuePair(uint32_t index, uint32_t bytes_of_key_value_data,
                     const char* mem, KtxKeyValuePair* out) {
  uint32_t current_index = 0;

  uint32_t bytes_read = 0;
  while (current_index < index) {
    if (bytes_read >= bytes_of_key_value_data) {
      return false;
    }
    uint32_t key_and_value_byte_size =
        *reinterpret_cast<const uint32_t*>(mem + bytes_read);
    bytes_read += key_and_value_byte_size +
                  CalculateKeyValuePairPadding(key_and_value_byte_size);
    ++current_index;
  }

  out->key_and_value_byte_size =
      *reinterpret_cast<const uint32_t*>(mem + bytes_read);
  bytes_read += sizeof(uint32_t);
  out->key = mem + bytes_read;
  bytes_read += strlen(out->key);
  out->value = mem + bytes_read;
  bytes_read += strlen(out->value);
  out->padding = mem + bytes_read;
  return true;
}

}  // namespace ktx