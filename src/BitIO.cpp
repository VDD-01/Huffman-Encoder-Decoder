#include "BitIO.h"
#include <fstream>
#include <vector>

using namespace std;

BitWriter::BitWriter(ofstream& o) : out(o), buffer(0), bitCount(0) {}

void BitWriter::writeBit(bool bit) {
    // buffer = static_cast<uint8_t>((buffer << 1) | (bit ? 1 : 0));    //type casting into 8 bit unsigned int
    // bitCount++;
    // if (bitCount == 8) {
    //     out.put(static_cast<char>(buffer));
    //     buffer = 0;
    //     bitCount = 0;
    // }
    buffer = (buffer << 1) | bit; 
    
    if (++bitCount == 8) {
        out.put(static_cast<char>(buffer));
        buffer = 0;
        bitCount = 0;
    }
}

void BitWriter::writeBits(const vector<bool>& bits) {
    for (bool b : bits) {
        writeBit(b);
    }
}

void BitWriter::flush() {
    if (bitCount > 0) {
        buffer <<= (8 - bitCount);
        out.put(static_cast<char>(buffer));
        buffer = 0;
        bitCount = 0;
    }
}

BitReader::BitReader(ifstream& i) : in(i), buffer(0), bitCount(0) {}

bool BitReader::readBit(bool& bit) {
    if (bitCount == 0) {
        int c = in.get();
        if (c == EOF) {
            return false;
        }
        buffer = static_cast<uint8_t>(c);
        bitCount = 8;
    }
    // Extracts the leftmost bit (MSB) by masking the buffer with 10000000 and checking if the result is non-zero.
    bit = (buffer & 0x80) != 0;
    buffer <<= 1;
    bitCount--;
    return true;
}
