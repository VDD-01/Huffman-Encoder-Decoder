#include "LZ77.h"
#include <cstdint>
#include <vector>

using namespace std;

namespace LZ77 {

vector<uint8_t> compress(const vector<uint8_t>& data) {
    const int windowSize = 2048;
    const int maxLength = 255;
    const int minMatch = 3;

    vector<uint8_t> out;
    size_t i = 0;

    while (i < data.size()) {
        int bestLength = 0;
        int bestOffset = 0;

        size_t start = (i > static_cast<size_t>(windowSize)) ? i - windowSize : 0;
        for (size_t j = start; j < i; ++j) {
            int length = 0;
            while (length < maxLength && i + length < data.size() && data[j + length] == data[i + length]) {
                length++;
            }
            if (length > bestLength) {
                bestLength = length;
                bestOffset = static_cast<int>(i - j);
            }
        }

        if (bestLength >= minMatch) {
            out.push_back(1); // match token
            uint16_t offset = static_cast<uint16_t>(bestOffset);
            out.push_back(static_cast<uint8_t>(offset & 0xFF));
            out.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(bestLength));
            i += bestLength;
        } else {
            out.push_back(0); // literal token
            out.push_back(data[i]);
            i++;
        }
    }

    return out;
}

vector<uint8_t> decompress(const vector<uint8_t>& data) {
    vector<uint8_t> out;
    size_t i = 0;

    while (i < data.size()) {
        uint8_t flag = data[i++];
        if (flag == 0) {
            if (i >= data.size()) {
                break;
            }
            out.push_back(data[i++]);
        } else if (flag == 1) {
            if (i + 2 >= data.size()) {
                break;
            }
            uint16_t offset = static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i + 1]) << 8);
            uint8_t length = data[i + 2];
            i += 3;

            if (offset == 0 || offset > out.size()) {
                break;
            }

            size_t start = out.size() - offset;
            for (int k = 0; k < length; ++k) {
                out.push_back(out[start + k]);
            }
        } else {
            break;
        }
    }

    return out;
}

} // namespace LZ77
