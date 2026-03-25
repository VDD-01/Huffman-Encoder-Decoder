#include <iostream>
#include <string>
#include "Huffman.h"

using namespace std;

void printUsage(const string& exe) {
    cout << "Usage:\n";
    cout << "  " << exe << " c <input> <output>   Compress file\n";
    cout << "  " << exe << " d <input> <output>   Decompress file\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    string mode = argv[1];
    string input = argv[2];
    string output = argv[3];

    if (mode == "c") {
        if (!Huffman::compressFile(input, output)) {
            return 1;
        }
    } else if (mode == "d") {
        if (!Huffman::decompressFile(input, output)) {
            return 1;
        }
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
