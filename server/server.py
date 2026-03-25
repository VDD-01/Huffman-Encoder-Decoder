from flask import Flask, request, send_file
import os
import shutil
import subprocess
import tempfile

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
STATIC_DIR = os.path.join(BASE_DIR, "web")


def find_executable():
    candidates = ["huffman.exe", "huffman"]
    for name in candidates:
        path = os.path.join(BASE_DIR, name)
        if os.path.isfile(path):
            return path
    return None


def run_huffman(mode, input_path, output_path):
    exe = find_executable()
    if not exe:
        raise RuntimeError("Huffman executable not found. Build with the provided compile command.")
    subprocess.run([exe, mode, input_path, output_path], check=True)


app = Flask(__name__, static_folder=STATIC_DIR, static_url_path="")


@app.route("/")
def index():
    return app.send_static_file("index.html")


@app.post("/compress")
def compress():
    if "file" not in request.files:
        return "No file provided", 400

    file = request.files["file"]
    if file.filename == "":
        return "No file selected", 400

    temp_dir = tempfile.mkdtemp(prefix="huffman_")
    input_path = os.path.join(temp_dir, file.filename)
    output_name = file.filename + ".huf"
    output_path = os.path.join(temp_dir, output_name)

    file.save(input_path)

    try:
        run_huffman("c", input_path, output_path)
    except Exception as exc:
        shutil.rmtree(temp_dir, ignore_errors=True)
        return f"Compression failed: {exc}", 500

    response = send_file(output_path, as_attachment=True, download_name=output_name)
    response.headers["X-Output-Name"] = output_name

    @response.call_on_close
    def cleanup():
        shutil.rmtree(temp_dir, ignore_errors=True)

    return response


@app.post("/decompress")
def decompress():
    if "file" not in request.files:
        return "No file provided", 400

    file = request.files["file"]
    if file.filename == "":
        return "No file selected", 400

    temp_dir = tempfile.mkdtemp(prefix="huffman_")
    input_path = os.path.join(temp_dir, file.filename)
    if file.filename.endswith(".huf"):
        output_name = file.filename[:-4]
    else:
        output_name = file.filename + ".out"
    output_path = os.path.join(temp_dir, output_name)

    file.save(input_path)

    try:
        run_huffman("d", input_path, output_path)
    except Exception as exc:
        shutil.rmtree(temp_dir, ignore_errors=True)
        return f"Decompression failed: {exc}", 500

    response = send_file(output_path, as_attachment=True, download_name=output_name)
    response.headers["X-Output-Name"] = output_name

    @response.call_on_close
    def cleanup():
        shutil.rmtree(temp_dir, ignore_errors=True)

    return response


if __name__ == "__main__":
    app.run(debug=True)
