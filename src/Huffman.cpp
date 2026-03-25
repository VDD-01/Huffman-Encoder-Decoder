#include "Huffman.h"
#include "BitIO.h"
#include "LZ77.h"
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

using namespace std;

namespace Huffman {

Node::Node(uint64_t f, int s) : freq(f), symbol(s) {}
Node::Node(uint64_t f, unique_ptr<Node> l, unique_ptr<Node> r)
    : freq(f), symbol(-1), left(move(l)), right(move(r)) {}

bool Node::isLeaf() const {
    return !left && !right;
}

struct NodeCompare {
    bool operator()(const unique_ptr<Node>& a, const unique_ptr<Node>& b) const {
        return a->freq > b->freq;
    }
};

static unique_ptr<Node> buildTree(const array<uint64_t, 256>& freqs) {
    priority_queue<unique_ptr<Node>, vector<unique_ptr<Node>>, NodeCompare> pq;

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            pq.push(make_unique<Node>(freqs[i], i));
        }
    }

    if (pq.empty()) {
        return nullptr;
    }

    if (pq.size() == 1) {
        auto only = move(const_cast<unique_ptr<Node>&>(pq.top()));
        pq.pop();
        auto dummy = make_unique<Node>(0, -1);
        return make_unique<Node>(only->freq, move(only), move(dummy));
    }

    while (pq.size() > 1) {
        auto left = move(const_cast<unique_ptr<Node>&>(pq.top()));
        pq.pop();
        auto right = move(const_cast<unique_ptr<Node>&>(pq.top()));
        pq.pop();
        uint64_t sum = left->freq + right->freq;
        pq.push(make_unique<Node>(sum, move(left), move(right)));
    }

    auto root = move(const_cast<unique_ptr<Node>&>(pq.top()));
    pq.pop();
    return root;
}

static void buildCodes(const Node* node, vector<bool>& path, array<vector<bool>, 256>& codes) {
    if (!node) return;

    if (node->isLeaf()) {
        if (node->symbol >= 0) {
            codes[static_cast<size_t>(node->symbol)] = path;
        }
        return;
    }

    path.push_back(false);
    buildCodes(node->left.get(), path, codes);
    path.pop_back();

    path.push_back(true);
    buildCodes(node->right.get(), path, codes);
    path.pop_back();
}

static bool writeHeader(ofstream& out, uint64_t originalSize, uint64_t lzSize, const array<uint64_t, 256>& freqs) {
    const char magic[4] = {'H', 'F', 'Z', '1'};
    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));
    out.write(reinterpret_cast<const char*>(&lzSize), sizeof(lzSize));
    for (uint64_t f : freqs) {
        out.write(reinterpret_cast<const char*>(&f), sizeof(uint64_t));
    }
    return static_cast<bool>(out);
}

static bool readHeader(ifstream& in, uint64_t& originalSize, uint64_t& lzSize, array<uint64_t, 256>& freqs) {
    char magic[4];
    in.read(magic, 4);
    if (!in || magic[0] != 'H' || magic[1] != 'F' || magic[2] != 'Z' || magic[3] != '1') {
        return false;
    }
    in.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
    in.read(reinterpret_cast<char*>(&lzSize), sizeof(lzSize));
    if (!in) return false;
    for (uint64_t& f : freqs) {
        in.read(reinterpret_cast<char*>(&f), sizeof(uint64_t));
        if (!in) return false;
    }
    return true;
}

static bool decodeHuffmanStream(ifstream& in, const Node* root, uint64_t count, vector<uint8_t>& outBytes) {
    if (count == 0) {
        return true;
    }
    if (!root) {
        return false;
    }

    BitReader reader(in);
    const Node* node = root;

    while (outBytes.size() < count) {
        bool bit = false;
        if (!reader.readBit(bit)) {
            return false;
        }
        node = bit ? node->right.get() : node->left.get();
        if (node && node->isLeaf()) {
            if (node->symbol >= 0) {
                outBytes.push_back(static_cast<uint8_t>(node->symbol));
                node = root;
            }
        }
    }

    return true;
}

bool compressFile(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Failed to open input file: " << inputPath << "\n";
        return false;
    }

    vector<uint8_t> data;
    char c;
    while (in.get(c)) {
        data.push_back(static_cast<uint8_t>(c));
    }

    vector<uint8_t> lzData = LZ77::compress(data);
    uint64_t originalSize = data.size();
    uint64_t lzSize = lzData.size();

    array<uint64_t, 256> freqs{};
    for (uint8_t b : lzData) {
        freqs[b]++;
    }

    auto root = buildTree(freqs);
    array<vector<bool>, 256> codes;
    if (root) {
        vector<bool> path;
        buildCodes(root.get(), path, codes);
    }

    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Failed to open output file: " << outputPath << "\n";
        return false;
    }

    if (!writeHeader(out, originalSize, lzSize, freqs)) {
        cerr << "Failed to write header.\n";
        return false;
    }

    BitWriter writer(out);
    if (root) {
        for (uint8_t byte : lzData) {
            writer.writeBits(codes[byte]);
        }
    }
    writer.flush();
    return true;
}

bool decompressFile(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Failed to open input file: " << inputPath << "\n";
        return false;
    }

    uint64_t originalSize = 0;
    uint64_t lzSize = 0;
    array<uint64_t, 256> freqs{};
    if (!readHeader(in, originalSize, lzSize, freqs)) {
        cerr << "Invalid or corrupted header.\n";
        return false;
    }

    auto root = buildTree(freqs);
    vector<uint8_t> lzData;
    lzData.reserve(static_cast<size_t>(lzSize));

    if (!decodeHuffmanStream(in, root.get(), lzSize, lzData)) {
        cerr << "Failed to decode Huffman stream.\n";
        return false;
    }

    vector<uint8_t> data = LZ77::decompress(lzData);
    if (data.size() != originalSize) {
        cerr << "Decompressed size mismatch.\n";
        return false;
    }

    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Failed to open output file: " << outputPath << "\n";
        return false;
    }

    for (uint8_t b : data) {
        out.put(static_cast<char>(b));
    }

    return true;
}

} // namespace Huffman
