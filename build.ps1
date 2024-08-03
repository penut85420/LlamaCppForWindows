$CUDA_PATH = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.2"
$NVCC_PATH = "$CUDA_PATH/bin/nvcc.exe"
$env:CudaToolkitDir = "$CUDA_PATH"

# For GPU
cmake -B build . --fresh `
    -DCMAKE_BUILD_TYPE=Release `
    -DGGML_CUDA=ON -T="v142" `
    -DCUDAToolkit_ROOT="$CUDA_PATH" `
    -DCMAKE_CUDA_COMPILER="$NVCC_PATH" `
    -DCMAKE_CUDA_ARCHITECTURES="86"
cmake --build build --config Release -j

# For CPU
cmake -B build-cpu . --fresh -DCMAKE_BUILD_TYPE=Release -DGGML_CUDA=OFF
cmake --build build-cpu --config Release -j
