# YARVS - Yet Another RISC-V Simulator

## How to build with nix package manager

### 0) Requirements

Installed nix package manager.

### 1) Create development environment

```bash
nix develop
```

### 2) Build the project

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build [--target <tgt>]
```

## How to build with conan package manager

### 0) Requirements

The following applications and libraries have to be installed:

- CMake of version 3.21 (or higher)
- python3
- python3-venv
- pip

### 1) Installing conan and other python modules this project depends on

```bash
python3 -m venv .venv
.venv/bin/pip3 install -r requirements.txt
```

### 2) Build the project

```bash
conan install . --output-folder=third_party --build=missing -s compiler.cppstd=23
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./third_party/conan_toolchain.cmake
cmake --build build [--target <tgt>]
```
