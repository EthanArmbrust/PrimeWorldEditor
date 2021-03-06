# Prime World Editor
Prime World Editor is a custom editor suite for Retro Studios' GameCube and Wii games, including the Metroid Prime series and Donkey Kong Country Returns.

# Build Requirements
On Windows:
* Visual Studio 2017 64-bit
* Qt 5.10 installation
* Qt Creator
* LLVM 6.0.1 installation; currently must be installed to `C:\Program Files\LLVM\`
* Python 3+
* Python packages `clang` and `mako`
* You'll also need to download glew 2.1.0 and extract it into the externals folder (so you have `externals/glew-2.1.0/`).

In addition you'll need to compile assimp and nod separately with CMake. The build system is kind of a pain right now, sorry.

Most of the project code is cross-platform, but currently has only been tested on Windows, and parts of the build process most likely will not work correctly on other platforms.
