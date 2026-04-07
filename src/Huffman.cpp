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

//move transfers the ownership of the resource(node) from one uniq_ptr to another

namespace Huffman {

Node::Node(uint64_t f, int s) : freq(f), symbol(s) {}
Node::Node(uint64_t f, unique_ptr<Node> l, unique_ptr<Node> r)
    : freq(f), symbol(-1), left(move(l)), right(move(r)) {}

bool Node::isLeaf() const {
    return !left && !right;
}

struct NodeCompare {
    bool operator()(const unique_ptr<Node>& a, const unique_ptr<Node>& b) const {
        return a->freq > b->freq;   //condition for min heap
    }
};

static unique_ptr<Node> buildTree(const array<uint64_t, 256>& freqs) {
    priority_queue<unique_ptr<Node>, vector<unique_ptr<Node>>, NodeCompare> pq;
    //arg1: type of elements in pq
    //arg2: underlying container
    //arg3: comparing condition

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            pq.push(make_unique<Node>(freqs[i], i));   //make_unique is used to create a unique_ptr to a new Node object
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

    while (pq.size() > 1) {//extracting the two nodes with the smallest frequencies and merging them
        auto left = move(const_cast<unique_ptr<Node>&>(pq.top()));
        //const_cast is used to remove the constness of the top element of the priority queue (pq always returns a const reference to the top element)
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


//codes is the lookup table for the huffman codes
//this method performs recursive dfs to build codes
//codes array is just adjacency lists of the huffman tree
static void buildCodes(const Node* node, vector<bool>& path, array<vector<bool>, 256>& codes) {
    if (!node) return;

    if (node->isLeaf()) {
        if (node->symbol >= 0) {
            codes[static_cast<size_t>(node->symbol)] = path;   //static cast is used for safe type conversion
        }
        return;
    }

    path.push_back(false);
    buildCodes(node->left.get(), path, codes);
    path.pop_back(); //this is backtracking- removing the last 0 cz we have reached the leftmost null node in dfs

    path.push_back(true);
    buildCodes(node->right.get(), path, codes);
    path.pop_back(); //this is backtracking- removing the last 1 cz we have reached the rightmost null node in dfs
}

static bool writeHeader(ofstream& out, uint64_t originalSize, uint64_t lzSize, const array<uint64_t, 256>& freqs) {
    const char magic[4] = {'M', 'I', 'T', '1'};
    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));//forced type conversion
    //out.write() expects a const char* as the first argument but originalSize is a uint64_t so we reinterpret_cast it to const char*
    out.write(reinterpret_cast<const char*>(&lzSize), sizeof(lzSize));
    for (uint64_t f : freqs) {
        out.write(reinterpret_cast<const char*>(&f), sizeof(uint64_t));
    }
    return static_cast<bool>(out);
    //If the file was written to successfully, the stream stays in a "good" state and returns true
}

//header contains the 4byte magic code, 8byte original size, 8byte lz size, 256*8byte frequency table

static bool readHeader(ifstream& in, uint64_t& originalSize, uint64_t& lzSize, array<uint64_t, 256>& freqs) {
    char magic[4];
    in.read(magic, 4);
    if (!in || magic[0] != 'M' || magic[1] != 'I' || magic[2] != 'T' || magic[3] != '1') {
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
