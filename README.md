# Huffman Encoder/Decoder (C++)

A simple Huffman + LZ77 file compressor with a minimal web UI.

## Repo Layout

```
Huffman encoder/
├─ include/          # Header files
├─ src/              # C++ sources
├─ web/              # Frontend (HTML/CSS/JS)
├─ server/           # Flask backend
├─ README.md
```

## Build (C++ executable)

From the repo root:

```bash
c++ -std=c++17 -O2 -Iinclude -o huffman src/main.cpp src/Huffman.cpp src/LZ77.cpp src/BitIO.cpp
```

## Run the web app

Install Flask once:

```bash
pip install flask
```

Start the server:

```bash
python server/server.py
```

Then open:

```
http://localhost:5000
```

## CLI Usage

Compress:

```bash
./huffman c input.csv output.huf
```

Decompress:

```bash
./huffman d output.huf recovered.csv
```

## How it works (short)

1. LZ77 finds repeated patterns.
2. Huffman coding compresses the LZ77 stream.
3. A small header lets the decoder rebuild the tree.
