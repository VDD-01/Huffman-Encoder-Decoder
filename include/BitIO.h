#ifndef BITIO_H
#define BITIO_H

#include <cstdint>
#include <fstream>
#include <vector>

//works after huffman code generation of each character
class BitWriter {   //class to write huffman codes as bits to a file
public:
    explicit BitWriter(std::ofstream& o);
    void writeBit(bool bit);
    void writeBits(const std::vector<bool>& bits);
    void flush();

private:
    std::ofstream& out;
    uint8_t buffer;
    int bitCount;
};

class BitReader {
public:
    explicit BitReader(std::ifstream& i);
    bool readBit(bool& bit);

private:
    std::ifstream& in;
    uint8_t buffer;
    int bitCount;
};

#endif
