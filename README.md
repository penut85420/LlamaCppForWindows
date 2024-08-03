# Llama Cpp Windows Project

## Visual Studio Build Tools

CUDA 的 `nvcc` 編譯器在 Windows 上只支援 MSVC

先裝 MSVC 再裝 CUDA

電腦不需要有 GPU 也能編譯 CUDA 程式

[下載 Visual Studio](https://visualstudio.microsoft.com/zh-hant/downloads/) > Visual Studio 工具 > Build Tools for Visual Studio 2022

安裝「使用 C++ 的桌面開發」，額外勾選「MSVC v142 - VS 2019 C++ x64/x86 建置工具 (v14.29)」，為了支援 CUDA 12.2

約會用掉 8.26 GB 的硬碟空間

透過 Visual Studio Installer 可以刪掉所有東西

在 VS Code 裡面操作 MSVC Tools

```ps1
cd C:\path\to\project; code .
```

測試 `cl` 跟 `cmake` 指令存在：

```ps1
PS > cl
Microsoft (R) C/C++ Optimizing Compiler Version 19.40.33813 for x86
Copyright (C) Microsoft Corporation.  著作權所有，並保留一切權利。

使用方式: cl [ option... ] filename... [ /link linkoption... ]

PS > cmake
Usage

  cmake [options] <path-to-source>
  cmake [options] <path-to-existing-build>
  cmake [options] -S <path-to-source> -B <path-to-build>

Specify a source directory to (re-)generate a build system for it in the
current working directory.  Specify an existing build directory to
re-generate its build system.

Run 'cmake --help' for more information.
```

## CUDA Toolkit

通常只需要安裝 Runtime, Development, VS Integration 就好，這些檔案約需要 3 GB 左右的硬碟空間：

![cuda-00](assets/cuda-00.png)

如果出現這個畫面，那等等還有第二個步驟要做：

![cuda-01](assets/cuda-01.png)

前往以下路徑，請根據自身的安裝路徑與 CUDA 版本調整：

```txt
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\extras\visual_studio_integration\MSBuildExtensions
```

這個資料夾裡面包含四個檔案：

```txt
CUDA 12.2.props
CUDA 12.2.targets
CUDA 12.2.xml
Nvda.Build.CudaTasks.v12.2.dll
```

把這些檔案複製一份到以下路徑：

```txt
C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Microsoft\VC\v160\BuildCustomizations
C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Microsoft\VC\v170\BuildCustomizations
```

保險一點就是 v160 跟 v170 都各放一份，而且只放特定某個版本的，如果 `12.2` 跟 `11.8` 同時存在之類的還是容易有問題，詳細的編譯器對應請參考 [CUDA 官方文件](https://docs.nvidia.com/cuda/cuda-installation-guide-microsoft-windows/index.html)。

如果不需要 CUDA 了，可以把以下項目刪除：

![cuda-02](assets/cuda-02.png)

## VS Code Plugin

[C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack) 包含 C & C++ 的 IntelliSense, Themes 與 CMake 支援等等。

## Project Setup

```sh
git init
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp -b b3347 --depth 1
git submodule add https://github.com/ggerganov/llama.cpp.git llama.cpp
git commit -m "add llama.cpp as submodule"
```

## Coding

看著 `llama.cpp/examples/simple/simple.cpp` 來寫

## Build

雖然 Log 有助於我們除錯，但有時實際給客戶的程式，不希望揭露太多資訊給對方，就會選擇把除錯訊息都隱藏起來。在 llama.cpp 裡面使用 `LOG_DISABLE_LOGS` 與 `NDEBUG` 兩個定義來控制除錯訊息，因此在 `CMakeLists.txt` 裡面可以這樣寫：

```cmake
add_definitions(-DLOG_DISABLE_LOGS)
add_definitions(-DNDEBUG)
```

編譯的指令相對複雜，建議寫成 PowerShell Script 來執行：

```sh
# Build.ps1

$CUDA_PATH = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.2"
$NVCC_PATH = "$CUDA_PATH/bin/nvcc.exe"
$env:CudaToolkitDir = "$CUDA_PATH"

cmake -B build . --fresh `
    -DCMAKE_BUILD_TYPE=Release `
    -DGGML_CUDA=ON -T="v142" `
    -DCUDAToolkit_ROOT="$CUDA_PATH" `
    -DCMAKE_CUDA_COMPILER="$NVCC_PATH" `
    -DCMAKE_CUDA_ARCHITECTURES="86"
```

`CMAKE_CUDA_ARCHITECTURES` 可以指定只編譯某些 GPU 架構，藉此減小編譯執行檔或函式庫的大小，86 是 RTX 30 系列，如果是 RTX 40 系列則為 89，可以在[官方網站](https://developer.nvidia.com/cuda-gpus)查詢特定型號 GPU 的架構代號。

`-T="v142"` 用來指定 MSVC 編譯器的版本，比較新的 CUDA 可以用 v143，比較舊的 CUDA 要用 v141，但這個設定實際上到底如何辨別，其實我也不知道 😥

筆者執行 PowerShell Script 的方式如下：

```sh
PowerShell -File Build.ps1
```

如果成功的話應該會看到以下訊息：

```txt
-- Found CUDAToolkit: C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.2/include (found version "12.2.91")
-- CUDA found
-- Using CUDA architectures: 86
-- The CUDA compiler identification is NVIDIA 12.2.91
-- Detecting CUDA compiler ABI info
-- Detecting CUDA compiler ABI info - done
-- Check for working CUDA compiler: C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.2/bin/nvcc.exe - skipped
-- Detecting CUDA compile features
-- Detecting CUDA compile features - done
```

以下訊息代表系統沒找到 CUDA 在哪裡，需要仔細檢查 CUDA 相關的路徑變數設定是否正確無誤：

```txt
-- Could not find nvcc, please set CUDAToolkit_ROOT.
CMake Warning at llama.cpp/ggml/src/CMakeLists.txt:352 (message):
  CUDA not found
```

還有一種問題，會噴很大量的錯誤訊息出來，其中有幾行應該會長的像這樣：

```txt
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\include\crt/host_config.h(157): fatal error C1189: #error: -- unsupported Microsoft Visual Studio version! Only the versions between 2017 and 2022 (inclusive) are supported! The nvcc flag '-allow-unsupported-compiler' can be used to override this version check; however, using an unsupported host compiler may cause compilation failure or incorrect run time execution.  Use at your own risk.
```

這是 `nvcc` 與 MSVC 編譯器版本對不起來的關係，需要確認官方文件，透過 `-T="v142"` 來指定正確的版本。

如果第一步設定成功了，就可以開始編譯：

```sh
cmake --build build --config Release
```

接著就看我破破的小筆電開始燃燒風扇，看編譯器熱血噴一堆 Warning 出來吧 🤣

> 編譯期間請記得幫筆電接電源線，並小心不要觸碰金屬材質的部份，避免燙傷 🔥

約莫十分鐘左右，終於編譯完成啦！

到以下路徑：

```txt
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\bin\
```

將這些檔案複製起來，與 `main.exe` 一起丟到有 GPU 的電腦裡面：

```txt
cublasLt64_12.dll
cudart64_12.dll
cublas64_12.dll
```
