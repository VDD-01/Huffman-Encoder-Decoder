#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <vector>


namespace LZ77 {

std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);

} // namespace LZ77

#endif
