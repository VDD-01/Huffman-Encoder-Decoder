const compressInput = document.getElementById("compress-input");
const compressBtn = document.getElementById("compress-btn");
const compressResult = document.getElementById("compress-result");
const compressSize = document.getElementById("compress-size");
const compressDownload = document.getElementById("compress-download");
const compressName = document.getElementById("compress-name");

const decompressInput = document.getElementById("decompress-input");
const decompressBtn = document.getElementById("decompress-btn");
const decompressResult = document.getElementById("decompress-result");
const decompressSize = document.getElementById("decompress-size");
const decompressDownload = document.getElementById("decompress-download");
const decompressName = document.getElementById("decompress-name");

const formatSize = (bytes) => {
  if (bytes === 0) return "0 B";
  const units = ["B", "KB", "MB", "GB"];
  const idx = Math.floor(Math.log(bytes) / Math.log(1024));
  const value = bytes / Math.pow(1024, idx);
  return value.toFixed(value < 10 ? 2 : 1) + " " + units[idx];
};

const uploadFile = async (endpoint, file) => {
  const formData = new FormData();
  formData.append("file", file);
  const response = await fetch(endpoint, {
    method: "POST",
    body: formData
  });

  if (!response.ok) {
    const message = await response.text();
    throw new Error(message || "Request failed");
  }

  const blob = await response.blob();
  const outputName = response.headers.get("X-Output-Name") || file.name;
  return { blob, outputName };
};

compressInput.addEventListener("change", () => {
  const hasFile = compressInput.files.length > 0;
  compressBtn.disabled = !hasFile;
  compressName.textContent = hasFile ? compressInput.files[0].name : "No file selected";
  compressResult.classList.add("hidden");
});

decompressInput.addEventListener("change", () => {
  const hasFile = decompressInput.files.length > 0;
  decompressBtn.disabled = !hasFile;
  decompressName.textContent = hasFile ? decompressInput.files[0].name : "No file selected";
  decompressResult.classList.add("hidden");
});

compressBtn.addEventListener("click", async () => {
  const file = compressInput.files[0];
  if (!file) return;

  compressBtn.disabled = true;
  compressBtn.textContent = "Compressing...";

  try {
    const result = await uploadFile("/compress", file);
    const url = URL.createObjectURL(result.blob);
    compressSize.textContent = formatSize(result.blob.size);
    compressDownload.href = url;
    compressDownload.download = result.outputName;
    compressResult.classList.remove("hidden");
  } catch (error) {
    alert(error.message || "Compression failed");
  } finally {
    compressBtn.disabled = false;
    compressBtn.textContent = "Compress";
  }
});

decompressBtn.addEventListener("click", async () => {
  const file = decompressInput.files[0];
  if (!file) return;

  decompressBtn.disabled = true;
  decompressBtn.textContent = "Decompressing...";

  try {
    const result = await uploadFile("/decompress", file);
    const url = URL.createObjectURL(result.blob);
    decompressSize.textContent = formatSize(result.blob.size);
    decompressDownload.href = url;
    decompressDownload.download = result.outputName;
    decompressResult.classList.remove("hidden");
  } catch (error) {
    alert(error.message || "Decompression failed");
  } finally {
    decompressBtn.disabled = false;
    decompressBtn.textContent = "Decompress";
  }
});
