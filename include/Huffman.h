#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace Huffman {

class Node {
public:
    uint64_t freq;
    int symbol; // 0-255 for leaves, -1 for internal nodes
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    Node(uint64_t f, int s);
    Node(uint64_t f, std::unique_ptr<Node> l, std::unique_ptr<Node> r);

    bool isLeaf() const;
};

bool compressFile(const std::string& inputPath, const std::string& outputPath);
bool decompressFile(const std::string& inputPath, const std::string& outputPath);

} // namespace Huffman

#endif
