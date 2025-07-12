# YARVS - Yet Another RISC-V Simulator

## How to clone the repository

```bash
git clone --recurse-submodules https://github.com/KetchuppOfficial/YARVS.git
```

## How to build with nix package manager

### 0) Requirements

Installed nix package manager.

### 1) Create development environment

```bash
nix develop
```

You may need to pass additional options to nix in case you do not have corresponding features set
in nix.conf.

```bash
nix develop --extra-experimental-features nix-command --extra-experimental-features flakes
```

### 2) Build the project

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## How to build with conan package manager

### 0) Requirements

The following applications and libraries have to be installed:

- CMake of version 3.21 (or higher)
- python3
- python3-venv

### 1) Install conan and other python modules this project depends on

```bash
python3 -m venv .venv
.venv/bin/pip3 install -r requirements.txt
```

### 2) Run conan

If you do not have conan profile, you may create a default one with the command

```bash
.venv/bin/conan profile detect
```

or specify your own profile by adding option `--profile:build=<your_profile>` to the following
command that installs all libraries the project depends on.

```bash
.venv/bin/conan install . --output-folder=third_party --build=missing -s compiler.cppstd=23
```

### 2) Build the project with cmake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./third_party/conan_toolchain.cmake
cmake --build build
```
